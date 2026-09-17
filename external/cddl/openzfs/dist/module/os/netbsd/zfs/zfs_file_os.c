/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * NetBSD file I/O for file vdevs, pool cache files, and send/receive streams.
 * Path opens own an uninstalled file object. Descriptor lookups own a file
 * reference, which can be released by a different LWP from the lookup.
 */

#include <sys/zfs_context.h>
#include <sys/zfs_file.h>
#include <sys/zfs_ioctl_os.h>
#include <sys/filedesc.h>
#include <sys/stat.h>
#include <sys/vfs_syscalls.h>

struct zfs_file {
	file_t		*zf_file;
	boolean_t	zf_uninstalled;
};

int
zfs_file_open(const char *path, int flags, int mode, cred_t *cr,
    zfs_file_t **fpp)
{
	struct pathbuf *pb;
	vnode_t *vp;
	file_t *fp;
	zfs_file_t *zf;
	int error, fmode;

	*fpp = NULL;
	if ((flags & O_ACCMODE) == O_ACCMODE)
		return (SET_ERROR(EINVAL));
	fmode = FFLAGS(flags);

	/* Native vn_open performs lookup and open with the calling LWP's cred. */
	pb = pathbuf_create(path);
	error = vn_open(NULL, pb, 0, fmode, mode, &vp, NULL, NULL);
	pathbuf_destroy(pb);
	if (error != 0)
		return (error);
	VOP_UNLOCK(vp);
	if (vp->v_type != VREG) {
		(void) vn_close(vp, fmode, cr);
		return (SET_ERROR(EACCES));
	}

	/*
	 * fgetdummy supplies the native file lock without allocating a user
	 * descriptor. vnops can then do offset locking and enforce I/O flags.
	 * Such files must use fputdummy, never closef (which uses file_cache).
	 */
	fp = fgetdummy();
	fp->f_type = DTYPE_VNODE;
	fp->f_flag = fmode & FMASK;
	fp->f_cred = kauth_cred_hold(cr);
	fp->f_vnode = vp;
	fp->f_ops = &vnops;
	zf = kmem_alloc(sizeof (*zf), KM_SLEEP);
	zf->zf_file = fp;
	zf->zf_uninstalled = B_TRUE;
	*fpp = zf;
	return (0);
}

void
zfs_file_close(zfs_file_t *zf)
{
	file_t *fp = zf->zf_file;

	if (zf->zf_uninstalled) {
		(void) fp->f_ops->fo_close(fp);
		kauth_cred_free(fp->f_cred);
		fputdummy(fp);
	} else {
		(void) closef(fp);
	}
	kmem_free(zf, sizeof (*zf));
}

static int
zfs_file_io(zfs_file_t *zf, void *buf, size_t len, off_t *offp,
    enum uio_rw rw, int flags, ssize_t *resid)
{
	file_t *fp = zf->zf_file;
	struct iovec iov = { .iov_base = buf, .iov_len = len };
	struct uio uio = {
		.uio_iov = &iov,
		.uio_iovcnt = 1,
		.uio_offset = *offp,
		.uio_resid = len,
		.uio_rw = rw,
	};
	int error;

	if (resid != NULL)
		*resid = len;
	if ((fp->f_flag & (rw == UIO_READ ? FREAD : FWRITE)) == 0)
		return (SET_ERROR(EBADF));
	if (len > SSIZE_MAX)
		return (SET_ERROR(EINVAL));
	UIO_SETUP_SYSSPACE(&uio);
	if (rw == UIO_READ)
		error = fp->f_ops->fo_read(fp, offp, &uio, fp->f_cred, flags);
	else
		error = fp->f_ops->fo_write(fp, offp, &uio, fp->f_cred, flags);

	/* Match native read/write semantics after a partial transfer. */
	if (uio.uio_resid != len &&
	    (error == ERESTART || error == EINTR || error == EWOULDBLOCK))
		error = 0;
	if (resid != NULL)
		*resid = uio.uio_resid;
	else if (error == 0 && uio.uio_resid != 0)
		error = EIO;
	return (error);
}

int
zfs_file_write(zfs_file_t *zf, const void *buf, size_t len, ssize_t *resid)
{
	return (zfs_file_io(zf, __UNCONST(buf), len, &zf->zf_file->f_offset,
	    UIO_WRITE, FOF_UPDATE_OFFSET, resid));
}

int
zfs_file_pwrite(zfs_file_t *zf, const void *buf, size_t len, loff_t off,
    uint8_t ashift, ssize_t *resid)
{
	(void) ashift;
	return (zfs_file_io(zf, __UNCONST(buf), len, &off, UIO_WRITE, 0, resid));
}

int
zfs_file_read(zfs_file_t *zf, void *buf, size_t len, ssize_t *resid)
{
	return (zfs_file_io(zf, buf, len, &zf->zf_file->f_offset,
	    UIO_READ, FOF_UPDATE_OFFSET, resid));
}

int
zfs_file_pread(zfs_file_t *zf, void *buf, size_t len, loff_t off, ssize_t *resid)
{
	return (zfs_file_io(zf, buf, len, &off, UIO_READ, 0, resid));
}

int
zfs_file_seek(zfs_file_t *zf, loff_t *offp, int whence)
{
	file_t *fp = zf->zf_file;

	if (fp->f_ops->fo_seek == NULL)
		return (SET_ERROR(ESPIPE));
	return (fp->f_ops->fo_seek(fp, *offp, whence, offp, FOF_UPDATE_OFFSET));
}

int
zfs_file_getattr(zfs_file_t *zf, zfs_file_attr_t *attr)
{
	struct stat st;
	int error;

	error = zf->zf_file->f_ops->fo_stat(zf->zf_file, &st);
	if (error != 0)
		return (error);
	attr->zfa_size = st.st_size;
	attr->zfa_mode = st.st_mode;
	return (0);
}

int
zfs_file_fsync(zfs_file_t *zf, int flags)
{
	file_t *fp = zf->zf_file;
	vnode_t *vp;
	int error, nflags = FSYNC_WAIT;

	if (fp->f_type != DTYPE_VNODE)
		return (SET_ERROR(EINVAL));
	if ((flags & O_DSYNC) != 0 && (flags & O_SYNC) == 0)
		nflags |= FSYNC_DATAONLY;
	vp = fp->f_vnode;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_FSYNC(vp, fp->f_cred, nflags, 0, 0);
	VOP_UNLOCK(vp);
	return (error);
}

int
zfs_file_deallocate(zfs_file_t *zf, loff_t offset, loff_t len)
{
	/* NetBSD has no generic file hole-punch operation. */
	return (SET_ERROR(EOPNOTSUPP));
}

loff_t
zfs_file_off(zfs_file_t *zf)
{
	return (zf->zf_file->f_offset);
}

int
zfs_file_unlink(const char *path)
{
	return (do_sys_unlink(path, UIO_SYSSPACE));
}

zfs_file_t *
zfs_file_get(int fd)
{
	file_t *fp = fd_getfile2(curproc, fd);
	zfs_file_t *zf;

	if (fp == NULL)
		return (NULL);
	zf = kmem_alloc(sizeof (*zf), KM_SLEEP);
	zf->zf_file = fp;
	zf->zf_uninstalled = B_FALSE;
	return (zf);
}

void
zfs_file_put(zfs_file_t *zf)
{
	zfs_file_close(zf);
}

void *
zfs_file_private(zfs_file_t *zf)
{
	file_t *fp = zf->zf_file;

	/* Only cloned ZFS control descriptors contain a zfsdev_state_t. */
	return (fp->f_ops == &zfsdev_fileops ? fp->f_data : NULL);
}

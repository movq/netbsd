/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Use the NetBSD osnet port's fd_clone mechanism for per-open control state.
 * OpenZFS needs that state for every control open, including event readers.
 * Nonzero device minors retain the old native zvol dispatch.
 */

#include <sys/zfs_context.h>
#include <sys/filedesc.h>
#include <sys/zfs_vfsops.h>
#include <sys/zfs_ioctl_impl.h>
#include <sys/zfs_ioctl_os.h>
#include <sys/zvol_os.h>
#include <zfs_gitrev.h>

int zfs_bmajor = -1;
int zfs_cmajor = -1;
static volatile unsigned int zfsdev_opens;
static struct sysctllog *zfsdev_sysctl_log;
static int zfs_ioctl_version = ZFS_IOCVER_OZFS;
static char zfs_module_version[] = ZFS_META_GITREV;

CTASSERT(sizeof (zfs_iocparm_t) == 24);
CTASSERT(offsetof(zfs_iocparm_t, zfs_cmd) == 8);

static int
zfsdev_fioctl(file_t *fp, u_long cmd, void *arg)
{
	zfs_iocparm_t *zp = arg;
	zfs_cmd_t *zc;
	void *uaddr;
	int error, copyerror;

	if (cmd != ZFS_IOC_OS(cmd))
		return (SET_ERROR(ENOTTY));
	if (zp->zfs_ioctl_version != ZFS_IOCVER_OZFS ||
	    zp->zfs_cmd_size != sizeof (zfs_cmd_t))
		return (SET_ERROR(EINVAL));

	uaddr = (void *)(uintptr_t)zp->zfs_cmd;
	zc = vmem_zalloc(sizeof (*zc), KM_SLEEP);
	error = copyin(uaddr, zc, sizeof (*zc));
	if (error == 0) {
		error = zfsdev_ioctl_common(ZFS_IOCREQ(cmd), zc, 0);
		/* Preserve the command result even when the handler failed. */
		copyerror = copyout(zc, uaddr, sizeof (*zc));
		if (error == 0)
			error = copyerror;
	}
	vmem_free(zc, sizeof (*zc));
	return (error);
}

static int
zfsdev_fclose(file_t *fp)
{
	zfsdev_state_destroy(fp);
	atomic_dec_uint(&zfsdev_opens);
	return (0);
}

const struct fileops zfsdev_fileops = {
	.fo_name = "zfs",
	.fo_read = fbadop_read,
	.fo_write = fbadop_write,
	.fo_ioctl = zfsdev_fioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = fnullop_poll,
	.fo_stat = fbadop_stat,
	.fo_close = zfsdev_fclose,
	.fo_kqfilter = fnullop_kqfilter,
	.fo_restart = fnullop_restart,
};

void
zfsdev_private_set_state(void *priv, zfsdev_state_t *zs)
{
	file_t *fp = priv;

	fp->f_data = zs;
}

zfsdev_state_t *
zfsdev_private_get_state(void *priv)
{
	file_t *fp = priv;

	return (fp->f_data);
}

static int
zfsdev_open(dev_t dev, int flags, int fmt, lwp_t *l)
{
	file_t *fp;
	int fd, error;

	if (minor(dev) != 0)
		return (zvol_open(dev, flags, fmt, l));
	if (fmt != S_IFCHR)
		return (SET_ERROR(ENXIO));
	error = fd_allocfile(&fp, &fd);
	if (error != 0)
		return (error);

	mutex_enter(&zfsdev_state_lock);
	error = zfsdev_state_init(fp);
	mutex_exit(&zfsdev_state_lock);
	if (error != 0) {
		fd_abort(curproc, fp, fd);
		return (error);
	}
	atomic_inc_uint(&zfsdev_opens);
	return (fd_clone(fp, fd, flags, &zfsdev_fileops, fp->f_data));
}

static int
zfsdev_close(dev_t dev, int flags, int fmt, lwp_t *l)
{
	/* Control descriptors close through zfsdev_fclose after cloning. */
	if (minor(dev) == 0)
		return (0);
	return (zvol_close(dev, flags, fmt, l));
}

static const struct bdevsw zfs_bdevsw = {
	.d_open = zfsdev_open,
	.d_close = zfsdev_close,
	.d_strategy = zvol_strategy,
	.d_ioctl = zvol_ioctl,
	.d_dump = nodump,
	.d_psize = nosize,
	.d_discard = nodiscard,
	.d_flag = D_DISK | D_MPSAFE,
};

static const struct cdevsw zfs_cdevsw = {
	.d_open = zfsdev_open,
	.d_close = zfsdev_close,
	.d_read = zvol_read,
	.d_write = zvol_write,
	.d_ioctl = zvol_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_DISK | D_MPSAFE,
};

int
zfsdev_busy(void)
{
	return (zfsdev_opens != 0);
}

int
zfsdev_attach(void)
{
	const struct sysctlnode *node;
	int error;

	error = devsw_attach("zfs", &zfs_bdevsw, &zfs_bmajor,
	    &zfs_cdevsw, &zfs_cmajor);
	if (error != 0)
		return (error);

	error = sysctl_createv(&zfsdev_sysctl_log, 0, NULL, &node, 0,
	    CTLTYPE_NODE, "zfs", SYSCTL_DESCR("ZFS"), NULL, 0, NULL, 0,
	    CTL_VFS, CTL_CREATE, CTL_EOL);
	if (error == 0)
		error = sysctl_createv(&zfsdev_sysctl_log, 0, &node, &node, 0,
		    CTLTYPE_NODE, "version", SYSCTL_DESCR("ZFS version"),
		    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error == 0)
		error = sysctl_createv(&zfsdev_sysctl_log, 0, &node, NULL,
		    CTLFLAG_READONLY, CTLTYPE_INT, "ioctl",
		    SYSCTL_DESCR("ZFS ioctl ABI version"),
		    NULL, 0, &zfs_ioctl_version, 0, CTL_CREATE, CTL_EOL);
	if (error == 0)
		error = sysctl_createv(&zfsdev_sysctl_log, 0, &node, NULL,
		    CTLFLAG_READONLY, CTLTYPE_STRING, "module",
		    SYSCTL_DESCR("ZFS module version"),
		    NULL, 0, zfs_module_version, sizeof (zfs_module_version),
		    CTL_CREATE, CTL_EOL);
	if (error != 0) {
		sysctl_teardown(&zfsdev_sysctl_log);
		devsw_detach(&zfs_bdevsw, &zfs_cdevsw);
	}
	return (error);
}

void
zfsdev_detach(void)
{
	ASSERT0(zfsdev_opens);
	devsw_detach(&zfs_bdevsw, &zfs_cdevsw);
	sysctl_teardown(&zfsdev_sysctl_log);
}

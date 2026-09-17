/* SPDX-License-Identifier: CDDL-1.0 */
/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2015 by Delphix. All rights reserved.
 * Copyright 2015, OmniTI Computer Consulting, Inc. All rights reserved.
 *
 * Native synthetic vnodes and snapshot mounts, adapted from NetBSD osnet.
 */
#include <sys/zfs_context.h>
#include <sys/zfs_ctldir.h>
#include <sys/zfs_vfsops.h>
#include <sys/zfs_ioctl_impl.h>
#include <sys/dirent.h>
#include <sys/dsl_dataset.h>
#include <sys/dsl_pool.h>
#include <sys/zap.h>
#include <sys/fstrans.h>
#include <sys/malloc.h>
#include <miscfs/genfs/genfs.h>

#define	ZFS_SNAPDIR_NAME	"snapshot"

struct zfsctl_root {
	timestruc_t zc_cmtime;
};

/* The key size distinguishes these nodes from ordinary DMU object keys. */
struct sfs_node_key {
	uint64_t parent_id;
	uint64_t id;
};

struct sfs_node {
	struct sfs_node_key key;
	lwp_t *mounting;		/* protected by the vnode interlock */
};

#define	VTOSFS(vp)	((struct sfs_node *)(vp)->v_data)
static int (**zfs_sfsop_p)(void *);

/*
 * Stabilize a covered mount using the same transaction/recheck protocol as
 * native namei.  The successful transaction and reference form a busy hold.
 */
static struct mount *
sfs_busy_mountedhere(vnode_t *vp)
{
	struct mount *mp;

	while ((mp = vp->v_mountedhere) != NULL) {
		fstrans_start(mp);
		if (fstrans_held(mp) && mp == vp->v_mountedhere &&
		    (mp->mnt_iflag & IMNT_GONE) == 0) {
			vfs_ref(mp);
			return (mp);
		}
		fstrans_done(mp);
	}
	return (NULL);
}

static int
sfs_snapshot_mount(vnode_t *vp, const char *snapname)
{
	zfsvfs_t *zfsvfs = vp->v_mount->mnt_data;
	struct vfsops *ops;
	struct mount *mp;
	char *path = PNBUF_GET();
	char *osname = PNBUF_GET();
	int error;

	dmu_objset_name(zfsvfs->z_os, path);
	if (snprintf(osname, MAXPATHLEN, "%s@%s", path, snapname) >=
	    MAXPATHLEN ||
	    snprintf(path, MAXPATHLEN, "%s/.zfs/snapshot/%s",
	    vp->v_mount->mnt_stat.f_mntonname, snapname) >= MAXPATHLEN) {
		error = ENAMETOOLONG;
		goto out;
	}
	ops = vfs_getopsbyname(MOUNT_ZFS);
	if (ops == NULL) {
		error = ENODEV;
		goto out;
	}
	mp = vfs_mountalloc(ops, vp);
	if (mp == NULL) {
		vfs_delref(ops);
		error = ENOMEM;
		goto out;
	}
	mp->mnt_stat.f_owner = 0;
	mp->mnt_flag = MNT_RDONLY | MNT_NOSUID | MNT_IGNORE;
	mutex_enter(mp->mnt_updating);
	error = set_statvfs_info(path, UIO_SYSSPACE, osname, UIO_SYSSPACE,
	    MOUNT_ZFS, mp, curlwp);
	if (error == 0)
		error = vfs_set_lowermount(mp, vp->v_mount);
	if (error == 0)
		error = zfs_domount_snapshot(mp, osname, VTOSFS(vp)->key.id);
	if (error != 0) {
		(void) vfs_set_lowermount(mp, NULL);
		mutex_exit(mp->mnt_updating);
		vfs_rele(mp);
		goto out;
	}

	/* The parent export also serves snapshot filehandles. */
	((zfsvfs_t *)mp->mnt_data)->z_parent = zfsvfs;
	mp->mnt_stat.f_fsidx = vp->v_mount->mnt_stat.f_fsidx;
	(void) VFS_STATVFS(mp, &mp->mnt_stat);
	vref(vp);
	mountlist_append(mp);
	vp->v_mountedhere = mp;
	mutex_exit(mp->mnt_updating);
out:
	PNBUF_PUT(osname);
	PNBUF_PUT(path);
	return (error);
}

static int
sfs_lookup_snapshot(vnode_t *dvp, struct componentname *cnp, vnode_t **vpp)
{
	zfsvfs_t *zfsvfs = dvp->v_mount->mnt_data;
	struct sfs_node_key key = { .parent_id = ZFSCTL_INO_SNAPDIR };
	char snapname[ZFS_MAX_DATASET_NAME_LEN];
	struct sfs_node *node;
	struct mount *mp;
	vnode_t *vp;
	int error;

	*vpp = NULL;
	if (cnp->cn_namelen >= sizeof (snapname))
		return (ENOENT);
	memcpy(snapname, cnp->cn_nameptr, cnp->cn_namelen);
	snapname[cnp->cn_namelen] = '\0';
	dsl_pool_config_enter(dmu_objset_pool(zfsvfs->z_os), FTAG);
	error = dsl_dataset_snap_lookup(dmu_objset_ds(zfsvfs->z_os),
	    snapname, &key.id);
	dsl_pool_config_exit(dmu_objset_pool(zfsvfs->z_os), FTAG);
	if (error != 0)
		return (error);
	error = vcache_get(zfsvfs->z_vfs, &key, sizeof (key), &vp);
	if (error != 0)
		return (error);

	mutex_enter(vp->v_interlock);
	node = VTOSFS(vp);
	if (node->mounting != NULL) {
		/* Recursive pathname lookup during mount may use the cover. */
		if (node->mounting == curlwp) {
			mutex_exit(vp->v_interlock);
			*vpp = vp;
			return (0);
		}
		mutex_exit(vp->v_interlock);
		vrele(vp);
		yield();
		return (ERESTART);
	}
	if (vp->v_mountedhere == NULL) {
		node->mounting = curlwp;
		mutex_exit(vp->v_interlock);
		VOP_UNLOCK(dvp);
		error = sfs_snapshot_mount(vp, snapname);
		if (vn_lock(dvp, LK_EXCLUSIVE) != 0) {
			vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY);
			error = ENOENT;
		}
		mutex_enter(vp->v_interlock);
		node->mounting = NULL;
		mutex_exit(vp->v_interlock);
		if (error != 0) {
			vrele(vp);
			return (error);
		}
	} else {
		mutex_exit(vp->v_interlock);
	}
	mp = sfs_busy_mountedhere(vp);
	if (mp == NULL) {
		vrele(vp);
		return (ERESTART);
	}
	error = VFS_ROOT(mp, LK_EXCLUSIVE, vpp);
	if (error == 0) {
		/* Allow '..' and NFS traversal through the parent filesystem. */
		(*vpp)->v_vflag &= ~VV_ROOT;
		VOP_UNLOCK(*vpp);
	}
	vfs_unbusy(mp);
	vrele(vp);
	return (error);
}

static int
sfs_lookup(void *v)
{
	struct vop_lookup_v2_args *ap = v;
	vnode_t *dvp = ap->a_dvp;
	struct componentname *cnp = ap->a_cnp;
	struct sfs_node *node = VTOSFS(dvp);
	zfsvfs_t *zfsvfs = dvp->v_mount->mnt_data;
	int error;

	*ap->a_vpp = NULL;
	if ((error = zfs_enter(zfsvfs, FTAG)) != 0)
		return (error);
	if ((cnp->cn_flags & ISLASTCN) && cnp->cn_nameiop != LOOKUP) {
		error = EROFS;
	} else if (cnp->cn_namelen == 1 && cnp->cn_nameptr[0] == '.') {
		vref(dvp);
		*ap->a_vpp = dvp;
		error = 0;
	} else if (cnp->cn_flags & ISDOTDOT) {
		if (node->key.parent_id == 0)
			error = vcache_get(zfsvfs->z_vfs, &zfsvfs->z_root,
			    sizeof (zfsvfs->z_root), ap->a_vpp);
		else if (node->key.parent_id == ZFSCTL_INO_ROOT)
			error = zfsctl_root(zfsvfs, ap->a_vpp);
		else
			error = zfsctl_snapshot(zfsvfs, ap->a_vpp);
	} else if (node->key.parent_id == 0 &&
	    cnp->cn_namelen == strlen(ZFS_SNAPDIR_NAME) &&
	    memcmp(cnp->cn_nameptr, ZFS_SNAPDIR_NAME, cnp->cn_namelen) == 0) {
		error = zfsctl_snapshot(zfsvfs, ap->a_vpp);
	} else if (node->key.parent_id == ZFSCTL_INO_ROOT) {
		error = sfs_lookup_snapshot(dvp, cnp, ap->a_vpp);
	} else {
		error = ENOENT;
	}
	zfs_exit(zfsvfs, FTAG);
	return (error);
}

static int
sfs_open(void *v)
{
	struct vop_open_args *ap = v;

	return ((ap->a_mode & FWRITE) ? EACCES : 0);
}

static int
sfs_access(void *v)
{
	struct vop_access_args *ap = v;

	return ((ap->a_accmode & VWRITE) ? EACCES : 0);
}

static int
sfs_getattr(void *v)
{
	struct vop_getattr_args *ap = v;
	vnode_t *vp = ap->a_vp;
	struct sfs_node *node = VTOSFS(vp);
	zfsvfs_t *zfsvfs = vp->v_mount->mnt_data;
	struct vattr *va = ap->a_vap;
	dsl_dataset_t *ds;
	uint64_t count;
	int error;

	if ((error = zfs_enter(zfsvfs, FTAG)) != 0)
		return (error);
	vattr_null(va);
	va->va_type = VDIR;
	va->va_mode = 0555;
	va->va_nlink = 2;
	va->va_uid = va->va_gid = 0;
	va->va_fsid = vp->v_mount->mnt_stat.f_fsid;
	va->va_fileid = node->key.id;
	va->va_blocksize = DEV_BSIZE;
	gethrestime(&va->va_atime);
	va->va_ctime = zfsvfs->z_ctldir->zc_cmtime;
	va->va_gen = va->va_flags = va->va_rdev = 0;
	va->va_bytes = va->va_filerev = 0;
	if (node->key.parent_id == 0) {
		va->va_nlink++;
	} else if (node->key.parent_id == ZFSCTL_INO_ROOT) {
		ds = dmu_objset_ds(zfsvfs->z_os);
		va->va_ctime = dmu_objset_snap_cmtime(zfsvfs->z_os);
		dsl_pool_config_enter(dmu_objset_pool(zfsvfs->z_os), FTAG);
		if (dsl_dataset_phys(ds)->ds_snapnames_zapobj != 0) {
			error = zap_count(dmu_objset_pool(zfsvfs->z_os)->
			    dp_meta_objset, dsl_dataset_phys(ds)->
			    ds_snapnames_zapobj, &count);
			if (error == 0)
				va->va_nlink += count;
		}
		dsl_pool_config_exit(dmu_objset_pool(zfsvfs->z_os), FTAG);
	}
	va->va_mtime = va->va_birthtime = va->va_ctime;
	va->va_size = va->va_nlink;
	zfs_exit(zfsvfs, FTAG);
	return (error);
}

/*
 * Offsets 0 and 1 describe dot entries; 2 starts the snapshot ZAP cursor.
 * Cookies always identify the next entry, including across short reads.
 */
static int
sfs_readdir(void *v)
{
	struct vop_readdir_args *ap = v;
	struct sfs_node *node = VTOSFS(ap->a_vp);
	zfsvfs_t *zfsvfs = ap->a_vp->v_mount->mnt_data;
	struct uio *uio = ap->a_uio;
	struct dirent *de;
	char name[ZFS_MAX_DATASET_NAME_LEN];
	uint64_t id, cookie;
	off_t offset, next;
	ssize_t resid = uio->uio_resid;
	int error, ncookies = 0, maxcookies = 0;
	off_t *cookies = NULL;

	if ((error = zfs_enter(zfsvfs, FTAG)) != 0)
		return (error);
	if (ap->a_eofflag != NULL)
		*ap->a_eofflag = 0;
	if (ap->a_ncookies != NULL) {
		*ap->a_ncookies = 0;
		*ap->a_cookies = NULL;
	}
	de = kmem_zalloc(sizeof (*de), KM_SLEEP);
	if (ap->a_ncookies != NULL) {
		maxcookies = resid / _DIRENT_MINSIZE(de);
		if (maxcookies != 0)
			cookies = malloc(maxcookies * sizeof (*cookies),
			    M_TEMP, M_WAITOK);
	}
	while (uio->uio_resid != 0) {
		offset = uio->uio_offset;
		next = (uint64_t)offset + 1;
		if (offset == 0) {
			strlcpy(name, ".", sizeof (name));
			id = node->key.id;
		} else if (offset == 1) {
			strlcpy(name, "..", sizeof (name));
			id = node->key.parent_id == 0 ? zfsvfs->z_root :
			    node->key.parent_id;
		} else if (node->key.parent_id == 0 && offset == 2) {
			strlcpy(name, ZFS_SNAPDIR_NAME, sizeof (name));
			id = ZFSCTL_INO_SNAPDIR;
		} else if (node->key.parent_id == ZFSCTL_INO_ROOT) {
			cookie = (uint64_t)offset - 2;
			dsl_pool_config_enter(dmu_objset_pool(zfsvfs->z_os),
			    FTAG);
			error = dmu_snapshot_list_next(zfsvfs->z_os,
			    sizeof (name), name, &id, &cookie, NULL);
			dsl_pool_config_exit(dmu_objset_pool(zfsvfs->z_os),
			    FTAG);
			if (error != 0)
				break;
			next = cookie + 2;
		} else {
			error = ENOENT;
			break;
		}
		de->d_fileno = id;
		de->d_type = DT_DIR;
		de->d_namlen = strlen(name);
		memcpy(de->d_name, name, de->d_namlen + 1);
		de->d_reclen = _DIRENT_SIZE(de);
		/* Clear padding before copying the record to userspace. */
		memset((char *)de + offsetof(struct dirent, d_name) +
		    de->d_namlen + 1, 0, de->d_reclen -
		    offsetof(struct dirent, d_name) - de->d_namlen - 1);
		if (uio->uio_resid < de->d_reclen) {
			error = uio->uio_resid == resid ? EINVAL : 0;
			break;
		}
		error = uiomove(de, de->d_reclen, uio);
		uio->uio_offset = error == 0 ? next : offset;
		if (error != 0)
			break;
		if (cookies != NULL) {
			ASSERT3S(ncookies, <, maxcookies);
			cookies[ncookies++] = next;
		}
	}
	if (error == ENOENT) {
		error = 0;
		if (ap->a_eofflag != NULL)
			*ap->a_eofflag = 1;
	}
	if (cookies != NULL) {
		if (error != 0 || ncookies == 0)
			free(cookies, M_TEMP);
		else {
			*ap->a_ncookies = ncookies;
			*ap->a_cookies = cookies;
		}
	}
	kmem_free(de, sizeof (*de));
	zfs_exit(zfsvfs, FTAG);
	return (error);
}

static int
sfs_inactive(void *v)
{
	struct vop_inactive_v2_args *ap = v;

	*ap->a_recycle =
	    VTOSFS(ap->a_vp)->key.parent_id == ZFSCTL_INO_SNAPDIR;
	return (0);
}

static int
sfs_reclaim(void *v)
{
	struct vop_reclaim_v2_args *ap = v;
	struct sfs_node *node = VTOSFS(ap->a_vp);

	ap->a_vp->v_data = NULL;
	VOP_UNLOCK(ap->a_vp);
	kmem_free(node, sizeof (*node));
	return (0);
}

static const struct vnodeopv_entry_desc zfs_sfsop_entries[] = {
	{ &vop_default_desc,	vn_default_error },
	{ &vop_parsepath_desc,	genfs_parsepath },
	{ &vop_lookup_desc,	sfs_lookup },
	{ &vop_open_desc,	sfs_open },
	{ &vop_close_desc,	genfs_nullop },
	{ &vop_access_desc,	sfs_access },
	{ &vop_getattr_desc,	sfs_getattr },
	{ &vop_lock_desc,	genfs_lock },
	{ &vop_unlock_desc,	genfs_unlock },
	{ &vop_readdir_desc,	sfs_readdir },
	{ &vop_inactive_desc,	sfs_inactive },
	{ &vop_reclaim_desc,	sfs_reclaim },
	{ &vop_seek_desc,	genfs_seek },
	{ &vop_putpages_desc,	genfs_null_putpages },
	{ &vop_islocked_desc,	genfs_islocked },
	{ &vop_print_desc,	genfs_nullop },
	{ &vop_pathconf_desc,	genfs_pathconf },
	{ NULL, NULL }
};

const struct vnodeopv_desc zfs_sfsop_opv_desc =
	{ &zfs_sfsop_p, zfs_sfsop_entries };

void
zfsctl_init(void)
{
}

void
zfsctl_fini(void)
{
}

int
zfsctl_loadvnode(vfs_t *mp, vnode_t *vp, const void *key, size_t len,
    const void **newkey)
{
	struct sfs_node *node;
	zfsvfs_t *zfsvfs = mp->mnt_data;

	if (len != sizeof (node->key))
		return (EINVAL);
	if (zfsvfs->z_ctldir == NULL || (mp->mnt_iflag & IMNT_UNMOUNT))
		return (ENOENT);
	node = kmem_zalloc(sizeof (*node), KM_SLEEP);
	memcpy(&node->key, key, len);
	vp->v_data = node;
	vp->v_op = zfs_sfsop_p;
	vp->v_tag = VT_ZFS;
	vp->v_type = VDIR;
	uvm_vnp_setsize(vp, 0);
	*newkey = &node->key;
	return (0);
}

int
zfsctl_vptofh(vnode_t *vp, fid_t *fid, size_t *size)
{
	zfid_short_t out = { .zf_len = SHORT_FID_LEN };
	uint64_t id = VTOSFS(vp)->key.id;

	if (*size < sizeof (out)) {
		*size = sizeof (out);
		return (E2BIG);
	}
	*size = sizeof (out);
	for (unsigned i = 0; i < sizeof (out.zf_object); i++)
		out.zf_object[i] = id >> (8 * i);
	/* Generation zero distinguishes synthetic nodes. */
	memcpy(fid, &out, sizeof (out));
	return (0);
}

int
zfsctl_root(zfsvfs_t *zfsvfs, vnode_t **vpp)
{
	struct sfs_node_key key = { 0, ZFSCTL_INO_ROOT };

	return (vcache_get(zfsvfs->z_vfs, &key, sizeof (key), vpp));
}

int
zfsctl_snapshot(zfsvfs_t *zfsvfs, vnode_t **vpp)
{
	struct sfs_node_key key = { ZFSCTL_INO_ROOT, ZFSCTL_INO_SNAPDIR };

	return (vcache_get(zfsvfs->z_vfs, &key, sizeof (key), vpp));
}

void
zfsctl_create(zfsvfs_t *zfsvfs)
{
	struct zfsctl_root *zc = kmem_alloc(sizeof (*zc), KM_SLEEP);
	vnode_t *vp;
	uint64_t crtime[2];

	VERIFY0(VFS_ROOT(zfsvfs->z_vfs, LK_EXCLUSIVE, &vp));
	VERIFY0(sa_lookup(VTOZ(vp)->z_sa_hdl, SA_ZPL_CRTIME(zfsvfs),
	    crtime, sizeof (crtime)));
	vput(vp);
	ZFS_TIME_DECODE(&zc->zc_cmtime, crtime);
	ASSERT3P(zfsvfs->z_ctldir, ==, NULL);
	zfsvfs->z_ctldir = zc;
}

void
zfsctl_destroy(zfsvfs_t *zfsvfs)
{
	kmem_free(zfsvfs->z_ctldir, sizeof (*zfsvfs->z_ctldir));
	zfsvfs->z_ctldir = NULL;
}

int
zfsctl_lookup_objset(vfs_t *mp, uint64_t id, zfsvfs_t **zfsvfsp)
{
	struct sfs_node_key key = { ZFSCTL_INO_SNAPDIR, id };
	struct mount *snapmp;
	vnode_t *vp;
	int error;

	*zfsvfsp = NULL;
	error = vcache_get(mp, &key, sizeof (key), &vp);
	if (error != 0)
		return (error);
	snapmp = sfs_busy_mountedhere(vp);
	vrele(vp);
	if (snapmp == NULL)
		return (ESTALE);
	*zfsvfsp = snapmp->mnt_data;
	return (0);	/* Caller releases the busy mount via zfs_vfs_rele. */
}

int
zfsctl_snapshot_unmount(const char *name, int flags)
{
	zfsvfs_t *zfsvfs;
	struct mount *mp;
	int error;

	if (strchr(name, '@') == NULL)
		return (0);
	error = getzfsvfs(name, &zfsvfs);
	if (error == ENOENT)
		return (0);
	if (error != 0)
		return (error);
	mp = zfsvfs->z_vfs;
	vfs_ref(mp);
	zfs_vfs_rele(zfsvfs);
	error = dounmount(mp, flags, curlwp);
	vfs_rele(mp);
	return (error);
}

int
zfsctl_umount_snapshots(vfs_t *mp, int flags, cred_t *cr)
{
	zfsvfs_t *zfsvfs = mp->mnt_data;
	struct sfs_node_key key = { .parent_id = ZFSCTL_INO_SNAPDIR };
	char name[ZFS_MAX_DATASET_NAME_LEN];
	struct mount *snapmp;
	vnode_t *vp;
	uint64_t cookie = 0;
	int error;

	for (;;) {
		dsl_pool_config_enter(dmu_objset_pool(zfsvfs->z_os), FTAG);
		error = dmu_snapshot_list_next(zfsvfs->z_os, sizeof (name),
		    name, &key.id, &cookie, NULL);
		dsl_pool_config_exit(dmu_objset_pool(zfsvfs->z_os), FTAG);
		if (error != 0)
			return (error == ENOENT ? 0 : error);
		error = vcache_get(mp, &key, sizeof (key), &vp);
		if (error == ENOENT)
			continue;
		if (error != 0)
			return (error);
		snapmp = sfs_busy_mountedhere(vp);
		if (snapmp != NULL) {
			vfs_ref(snapmp);
			vfs_unbusy(snapmp);
			error = dounmount(snapmp, flags, curlwp);
			vfs_rele(snapmp);
		}
		vrele(vp);
		if (error != 0 && error != ENOENT)
			return (error);
	}
}

boolean_t
zfsctl_is_node(vnode_t *vp)
{
	return (vp->v_op == zfs_sfsop_p);
}

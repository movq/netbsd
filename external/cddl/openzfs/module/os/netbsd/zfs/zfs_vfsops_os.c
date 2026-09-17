/* SPDX-License-Identifier: CDDL-1.0 */
/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Native mount and suspension operations, adapted from NetBSD's osnet port.
 * Dataset metadata initialization and property callbacks are shared with
 * the current FreeBSD implementation.
 */
#include <sys/zfs_context.h>
#include <sys/zfs_vfsops.h>
#include <sys/zfs_vcache.h>
#include <sys/zfs_vnops.h>
#include <sys/zfs_ctldir.h>
#include <sys/zfs_dir.h>
#include <sys/zfs_quota.h>
#include <sys/dmu_objset.h>
#include <sys/dsl_dataset.h>
#include <sys/dsl_dir.h>
#include <sys/dsl_prop.h>
#include <sys/dsl_deleg.h>
#include <sys/spa_impl.h>
#include <sys/zfeature.h>
#include <sys/fstrans.h>
#include <sys/mkdev.h>
#include <miscfs/genfs/genfs.h>

int zfs_super_owner;
static uint32_t zfs_active_fs_count;

extern const struct vnodeopv_desc zfs_vnodeop_opv_desc;
extern const struct vnodeopv_desc zfs_specop_opv_desc;
extern const struct vnodeopv_desc zfs_fifoop_opv_desc;
extern const struct vnodeopv_desc zfs_sfsop_opv_desc;
extern int zfs_netbsd_vptofh(vnode_t *, fid_t *, size_t *);
extern int zfs_netbsd_fhtovp(vfs_t *, fid_t *, int, vnode_t **);

static int zfs_mount(vfs_t *, const char *, void *, size_t *);
static int zfs_umount(vfs_t *, int);
static int zfs_root(vfs_t *, int, vnode_t **);
static int zfs_statvfs(vfs_t *, struct statvfs *);
static int zfs_sync(vfs_t *, int, cred_t *);
static int zfs_vget(vfs_t *, ino_t, int, vnode_t **);

static const struct vnodeopv_desc * const zfs_vnodeop_descs[] = {
	&zfs_vnodeop_opv_desc,
	&zfs_specop_opv_desc,
	&zfs_fifoop_opv_desc,
	&zfs_sfsop_opv_desc,
	NULL
};

static void
zfs_vfs_nop(void)
{
	/* zfs_kmod_init/fini own filesystem initialization, not vfs_attach. */
}

struct vfsops zfs_vfsops = {
	.vfs_name = MOUNT_ZFS,
	.vfs_min_mount_data = sizeof (struct zfs_args),
	.vfs_opv_descs = zfs_vnodeop_descs,
	.vfs_mount = zfs_mount,
	.vfs_unmount = zfs_umount,
	.vfs_root = zfs_root,
	.vfs_statvfs = zfs_statvfs,
	.vfs_sync = zfs_sync,
	.vfs_vget = zfs_vget,
	.vfs_loadvnode = zfs_loadvnode,
	.vfs_newvnode = zfs_newvnode,
	.vfs_init = zfs_vfs_nop,
	.vfs_done = zfs_vfs_nop,
	.vfs_start = (void *)nullop,
	.vfs_reinit = zfs_vfs_nop,
	.vfs_renamelock_enter = genfs_renamelock_enter,
	.vfs_renamelock_exit = genfs_renamelock_exit,
	.vfs_vptofh = zfs_netbsd_vptofh,
	.vfs_fhtovp = zfs_netbsd_fhtovp,
	.vfs_quotactl = (void *)eopnotsupp,
	.vfs_extattrctl = (void *)eopnotsupp,
	.vfs_suspendctl = genfs_suspendctl,
	.vfs_snapshot = (void *)eopnotsupp,
	.vfs_fsync = (void *)eopnotsupp
};

int
zfs_netbsd_check_mount(objset_t *os)
{
	/*
	 * Feature use persists after longname=off.  Check the dataset,
	 * including snapshots, rather than the property or pool feature.
	 * Administrative holds deliberately do not apply this restriction.
	 */
	if (dsl_dataset_feature_is_active(dmu_objset_ds(os),
	    SPA_FEATURE_LONGNAME))
		return (SET_ERROR(ENOTSUP));
	return (0);
}

static int
zfs_domount_impl(vfs_t *mp, const char *name, uint64_t snapid)
{
	zfsvfs_t *zfsvfs;
	objset_t *os;
	uint64_t fsid, value;
	int error;

	error = zfsvfs_create(name, (mp->mnt_flag & MNT_RDONLY) != 0,
	    &zfsvfs);
	if (error != 0)
		return (error);
	os = zfsvfs->z_os;
	zfsvfs->z_vfs = mp;
	mp->mnt_data = zfsvfs;
	/* A snapshot can be renamed/replaced while its automount is starting. */
	if (snapid != 0 && (!dmu_objset_is_snapshot(os) ||
	    dmu_objset_id(os) != snapid)) {
		error = SET_ERROR(ESTALE);
		goto fail;
	}
	error = zfs_netbsd_check_mount(os);
	if (error != 0)
		goto fail;

	error = dsl_prop_get_integer(name, "recordsize", &value, NULL);
	if (error != 0)
		goto fail;
	mp->mnt_stat.f_bsize = SPA_MINBLOCKSIZE;
	mp->mnt_stat.f_iosize = value;
	mp->mnt_flag |= MNT_LOCAL;
	mp->mnt_iflag |= IMNT_MPSAFE | IMNT_NCLOOKUP;
	fsid = dmu_objset_fsid_guid(os);
	mp->mnt_stat.f_fsidx.__fsid_val[0] = fsid;
	mp->mnt_stat.f_fsidx.__fsid_val[1] = ((fsid >> 32) << 8) |
	    (makefstype(mp->mnt_op->vfs_name) & 0xff);
	mp->mnt_stat.f_fsid = (uint32_t)fsid;

	if (dmu_objset_is_snapshot(os)) {
		zfsvfs->z_issnap = B_TRUE;
		zfsvfs->z_atime = B_FALSE;
		mp->mnt_flag |= MNT_RDONLY | MNT_NOATIME;
		os->os_sync = ZFS_SYNC_DISABLED;
		error = dsl_prop_get_integer(name, "xattr", &value, NULL);
		if (error != 0)
			goto fail;
		zfsvfs->z_xattr = value;
		zfsvfs->z_xattr_sa = value == ZFS_XATTR_SA;
		if (value != ZFS_XATTR_OFF)
			zfsvfs->z_flags |= ZSB_XATTR;
		error = dsl_prop_get_integer(name, "acltype", &value, NULL);
		if (error != 0)
			goto fail;
		zfsvfs->z_acl_type = value;
		mutex_enter(&os->os_user_ptr_lock);
		dmu_objset_set_user(os, zfsvfs);
		mutex_exit(&os->os_user_ptr_lock);
	} else {
		error = zfsvfs_setup(zfsvfs, B_TRUE);
		if (error != 0)
			goto fail;
		zfsctl_create(zfsvfs);
	}
	atomic_inc_32(&zfs_active_fs_count);
	return (0);

fail:
	/*
	 * setup may have registered callbacks before a later failure.
	 * It does not expose znodes until after its fallible allocations.
	 */
	zfs_unregister_callbacks(zfsvfs);
	if (zfsvfs->z_log != NULL)
		zil_close(zfsvfs->z_log);
	dmu_objset_disown(os, B_TRUE, zfsvfs);
	mp->mnt_data = NULL;
	zfsvfs_free(zfsvfs);
	return (error);
}

int
zfs_domount_snapshot(vfs_t *mp, const char *name, uint64_t snapid)
{
	ASSERT3U(snapid, !=, 0);
	return (zfs_domount_impl(mp, name, snapid));
}

static int
zfs_mount(vfs_t *mp, const char *path, void *data, size_t *data_len)
{
	struct zfs_args *args = data;
	vnode_t *covered = mp->mnt_vnodecovered;
	cred_t *cr = CRED();
	int error;

	/* Retain osnet's mount argument format and operation restrictions. */
	if (mp->mnt_flag & MNT_OP_FLAGS)
		return (SET_ERROR(ENOTSUP));
	if (args == NULL || *data_len < sizeof (*args))
		return (SET_ERROR(EINVAL));
	if (memchr(args->fspec, '\0', sizeof (args->fspec)) == NULL)
		return (SET_ERROR(ENAMETOOLONG));
	if (covered == NULL || covered->v_type != VDIR)
		return (SET_ERROR(ENOTDIR));

	error = secpolicy_fs_mount(cr, covered, mp);
	if (error != 0) {
		vattr_t va;

		if (dsl_deleg_access(args->fspec, ZFS_DELEG_PERM_MOUNT, cr))
			return (error);
		vn_lock(covered, LK_SHARED | LK_RETRY);
		error = VOP_GETATTR(covered, &va, cr);
		if (error == 0 &&
		    secpolicy_vnode_owner(covered, cr, va.va_uid) != 0)
			error = VOP_ACCESS(covered, VWRITE, cr);
		VOP_UNLOCK(covered);
		if (error != 0)
			return (error);
		secpolicy_fs_mount_clearopts(cr, mp);
	}

	/* Copy the user pathname before exposing or owning a dataset. */
	error = set_statvfs_info(path, UIO_USERSPACE, args->fspec,
	    UIO_SYSSPACE, MOUNT_ZFS, mp, curlwp);
	if (error != 0)
		return (error);
	return (zfs_domount_impl(mp, args->fspec, 0));
}

static int
zfs_statvfs(vfs_t *mp, struct statvfs *st)
{
	zfsvfs_t *zfsvfs = mp->mnt_data;
	uint64_t used, avail, objects, freeobjects;
	int error = zfs_enter(zfsvfs, FTAG);

	if (error != 0)
		return (error);
	dmu_objset_space(zfsvfs->z_os, &used, &avail, &objects, &freeobjects);
	st->f_bsize = st->f_frsize = SPA_MINBLOCKSIZE;
	st->f_iosize = mp->mnt_stat.f_iosize;
	st->f_blocks = (used + avail) >> SPA_MINBLOCKSHIFT;
	st->f_bfree = st->f_bavail = avail >> SPA_MINBLOCKSHIFT;
	st->f_bresvd = st->f_fresvd = 0;
	st->f_ffree = st->f_favail = MIN(freeobjects, st->f_bfree);
	st->f_files = st->f_ffree + objects;
	st->f_fsid = mp->mnt_stat.f_fsid;
	st->f_fsidx = mp->mnt_stat.f_fsidx;
	st->f_namemax = KERNEL_NAME_MAX;
	strlcpy(st->f_fstypename, MOUNT_ZFS, sizeof (st->f_fstypename));
	if (st != &mp->mnt_stat) {
		strlcpy(st->f_mntfromname, mp->mnt_stat.f_mntfromname,
		    sizeof (st->f_mntfromname));
		strlcpy(st->f_mntonname, mp->mnt_stat.f_mntonname,
		    sizeof (st->f_mntonname));
	}
	zfs_exit(zfsvfs, FTAG);
	return (0);
}

static int
zfs_getvnode(vfs_t *mp, uint64_t obj, int flags, vnode_t **vpp)
{
	zfsvfs_t *zfsvfs = mp->mnt_data;
	znode_t *zp;
	int error;

	*vpp = NULL;
	if ((error = zfs_enter(zfsvfs, FTAG)) != 0)
		return (error);
	error = zfs_zget(zfsvfs, obj, &zp);
	if (error == 0) {
		if (zp->z_unlinked || zp->z_sa_hdl == NULL) {
			vrele(ZTOV(zp));
			error = SET_ERROR(ESTALE);
		} else {
			*vpp = ZTOV(zp);
		}
	}
	zfs_exit(zfsvfs, FTAG);
	if (error == 0) {
		error = vn_lock(*vpp, flags);
		if (error != 0) {
			vrele(*vpp);
			*vpp = NULL;
		}
	}
	return (error);
}

static int
zfs_root(vfs_t *mp, int flags, vnode_t **vpp)
{
	return (zfs_getvnode(mp, ((zfsvfs_t *)mp->mnt_data)->z_root,
	    flags, vpp));
}

static int
zfs_vget(vfs_t *mp, ino_t ino, int flags, vnode_t **vpp)
{
	zfsvfs_t *zfsvfs = mp->mnt_data;

	*vpp = NULL;
	if (ino == ZFSCTL_INO_ROOT || ino == ZFSCTL_INO_SNAPDIR ||
	    (zfsvfs->z_shares_dir != 0 && ino == zfsvfs->z_shares_dir))
		return (SET_ERROR(EOPNOTSUPP));
	return (zfs_getvnode(mp, ino, flags, vpp));
}

static int
zfs_sync(vfs_t *mp, int waitfor, cred_t *cr)
{
	zfsvfs_t *zfsvfs;
	struct vnode_iterator *iter;
	vnode_t *vp;
	int error, allerror = 0;

	if (panicstr != NULL || waitfor == MNT_LAZY)
		return (0);
	if (mp == NULL) {
		spa_sync_allpools();
		return (0);
	}
	zfsvfs = mp->mnt_data;
	if ((mp->mnt_flag & MNT_RDONLY) == 0) {
		vfs_vnode_iterator_init(mp, &iter);
		while ((vp = vfs_vnode_iterator_next(iter, NULL, NULL)) != NULL) {
			if (!zfsctl_is_node(vp) &&
			    (vp->v_type == VREG || vp->v_type == VDIR)) {
				error = vn_lock(vp, LK_EXCLUSIVE);
				if (error == 0) {
					error = VOP_FSYNC(vp, cr,
					    waitfor == MNT_WAIT ? FSYNC_WAIT : 0,
					    0, 0);
					VOP_UNLOCK(vp);
				}
				if (error != 0)
					allerror = error;
			}
			vrele(vp);
		}
		vfs_vnode_iterator_destroy(iter);
	}
	if ((error = zfs_enter(zfsvfs, FTAG)) != 0)
		return (error);
	if (zfsvfs->z_log != NULL)
		error = zil_commit(zfsvfs->z_log, 0);
	zfs_exit(zfsvfs, FTAG);
	return (error != 0 ? error : allerror);
}

static int
zfs_umount(vfs_t *mp, int flags)
{
	zfsvfs_t *zfsvfs = mp->mnt_data;
	struct vnode_iterator *iter;
	vnode_t *vp;
	objset_t *os;
	int error;

	error = secpolicy_fs_unmount(CRED(), mp);
	if (error != 0 &&
	    dsl_deleg_access(mp->mnt_stat.f_mntfromname,
	    ZFS_DELEG_PERM_MOUNT, CRED()) != 0)
		return (error);
	if (zfsvfs->z_ctldir != NULL) {
		error = zfsctl_umount_snapshots(mp, flags, CRED());
		if (error != 0)
			return (error);
	}
	if (flags & MNT_FORCE) {
		ZFS_TEARDOWN_ENTER_WRITE(zfsvfs, FTAG);
		zfsvfs->z_unmounted = B_TRUE;
		ZFS_TEARDOWN_EXIT_WRITE(zfsvfs);
	}

	/* ZIL commit can instantiate more vnodes through zfs_get_data. */
	vfs_vnode_iterator_init(mp, &iter);
	while ((vp = vfs_vnode_iterator_next(iter, NULL, NULL)) != NULL) {
		vrele(vp);
		vfs_vnode_iterator_destroy(iter);
		error = vflush(mp, NULL, (flags & MNT_FORCE) ? FORCECLOSE : 0);
		if (error != 0)
			return (error);
		if (zfsvfs->z_log != NULL) {
			error = zil_commit(zfsvfs->z_log, 0);
			if (error != 0 && (flags & MNT_FORCE) == 0)
				return (error);
		}
		vfs_vnode_iterator_init(mp, &iter);
	}
	vfs_vnode_iterator_destroy(iter);
	VERIFY0(zfsvfs_teardown(zfsvfs, B_TRUE));
	os = zfsvfs->z_os;
	if (os != NULL) {
		mutex_enter(&os->os_user_ptr_lock);
		dmu_objset_set_user(os, NULL);
		mutex_exit(&os->os_user_ptr_lock);
		dmu_objset_disown(os, B_TRUE, zfsvfs);
	}
	if (zfsvfs->z_ctldir != NULL)
		zfsctl_destroy(zfsvfs);
	mp->mnt_data = NULL;
	zfsvfs_free(zfsvfs);
	atomic_dec_32(&zfs_active_fs_count);
	return (0);
}

int
zfs_suspend_fs(zfsvfs_t *zfsvfs)
{
	struct vnode_iterator *iter;
	vnode_t *vp;
	int error;

	error = vfs_suspend(zfsvfs->z_vfs, 0);
	if (error != 0)
		return (error);
	/*
	 * Finish and invalidate UVM data before detaching SA handles.
	 * Do not wait for pages under the ZFS teardown writer lock.
	 */
	vfs_vnode_iterator_init(zfsvfs->z_vfs, &iter);
	while ((vp = vfs_vnode_iterator_next(iter, NULL, NULL)) != NULL) {
		if (!zfsctl_is_node(vp) && vp->v_type == VREG) {
			vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
			rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
			error = VOP_PUTPAGES(vp, 0, 0, PGO_ALLPAGES |
			    PGO_CLEANIT | PGO_FREE | PGO_SYNCIO);
			VOP_UNLOCK(vp);
		}
		vrele(vp);
		if (error != 0)
			break;
	}
	vfs_vnode_iterator_destroy(iter);
	if (error == 0)
		error = zfsvfs_teardown(zfsvfs, B_FALSE);
	if (error != 0)
		vfs_resume(zfsvfs->z_vfs);
	return (error);
}

static bool
zfs_resume_selector(void *arg, vnode_t *vp)
{
	return (!zfsctl_is_node(vp) && VTOZ(vp)->z_sa_hdl == NULL);
}

static objset_t *
zfs_suspended_objset(zfsvfs_t *zfsvfs, dsl_dataset_t *ds)
{
	objset_t *os;
	dsl_pool_t *dp = spa_get_dsl(dsl_dataset_get_spa(ds));

	ASSERT(ZFS_TEARDOWN_WRITE_HELD(zfsvfs));
	ASSERT(ZFS_TEARDOWN_INACTIVE_WRITE_HELD(zfsvfs));
	VERIFY3P(ds->ds_owner, ==, zfsvfs);
	VERIFY(dsl_dataset_long_held(ds));
	dsl_pool_config_enter(dp, FTAG);
	VERIFY0(dmu_objset_from_ds(ds, &os));
	dsl_pool_config_exit(dp, FTAG);
	zfsvfs->z_os = os;
	return (os);
}

static void
zfs_resume_finish(zfsvfs_t *zfsvfs, boolean_t failed)
{
	struct vnode_iterator *iter;
	vnode_t *vp;

	if (failed) {
		/* The ioctl caller still holds a busy reference to this mount. */
		zfsvfs->z_unmounted = B_TRUE;
		mutex_enter(&zfsvfs->z_lock);
		zfsvfs->z_unmount_pending = B_TRUE;
		mutex_exit(&zfsvfs->z_lock);
	}
	ZFS_TEARDOWN_INACTIVE_EXIT_WRITE(zfsvfs);
	ZFS_TEARDOWN_EXIT_WRITE(zfsvfs);
	vfs_vnode_iterator_init(zfsvfs->z_vfs, &iter);
	while ((vp = vfs_vnode_iterator_next(iter, zfs_resume_selector,
	    NULL)) != NULL)
		vgone(vp);
	vfs_vnode_iterator_destroy(iter);
	vfs_resume(zfsvfs->z_vfs);
}

int
zfs_resume_fs(zfsvfs_t *zfsvfs, dsl_dataset_t *ds)
{
	objset_t *os = zfs_suspended_objset(zfsvfs, ds);
	znode_t *zp;
	int error;

	/* Online receive/rollback may have introduced long names. */
	error = zfs_netbsd_check_mount(os);
	if (error == 0)
		error = zfsvfs_init(zfsvfs, os);
	if (error == 0)
		error = zfsvfs_setup(zfsvfs, B_FALSE);
	if (error == 0) {
		ds->ds_dir->dd_activity_cancelled = B_FALSE;
		mutex_enter(&zfsvfs->z_znodes_lock);
		for (zp = list_head(&zfsvfs->z_all_znodes); zp != NULL;
		    zp = list_next(&zfsvfs->z_all_znodes, zp))
			(void) zfs_rezget(zp);
		mutex_exit(&zfsvfs->z_znodes_lock);
	}
	zfs_resume_finish(zfsvfs, error != 0);
	return (error);
}

int
zfs_end_fs(zfsvfs_t *zfsvfs, dsl_dataset_t *ds)
{
	(void) zfs_suspended_objset(zfsvfs, ds);
	zfs_resume_finish(zfsvfs, B_TRUE);
	return (0);
}

void
zfs_init(void)
{
	zfsctl_init();
	zfs_znode_init();
	dmu_objset_register_type(DMU_OST_ZFS, zpl_get_file_info);
}

void
zfs_fini(void)
{
	zfsctl_fini();
	zfs_znode_fini();
}

int
zfs_busy(void)
{
	return (zfs_active_fs_count != 0);
}

int
zfs_check_global_label(const char *dataset, const char *label)
{
	/* NetBSD has no Solaris labeled zones, as in osnet. */
	return (0);
}

void
zfsvfs_update_fromname(const char *oldname, const char *newname)
{
	struct mount *mp;
	struct mount_iterator *iter;
	size_t len = strlen(oldname);

	mountlist_iterator_init(&iter);
	while ((mp = mountlist_iterator_next(iter)) != NULL) {
		char *name = mp->mnt_stat.f_mntfromname;
		char replacement[MNAMELEN];

		if (strcmp(mp->mnt_op->vfs_name, MOUNT_ZFS) != 0 ||
		    strncmp(name, oldname, len) != 0 ||
		    (name[len] != '\0' && name[len] != '/' && name[len] != '@'))
			continue;
		snprintf(replacement, sizeof (replacement), "%s%s",
		    newname, name + len);
		strlcpy(name, replacement, MNAMELEN);
	}
	mountlist_iterator_destroy(iter);
}

/* SPDX-License-Identifier: BSD-2-Clause */

#include <sys/zfs_context.h>
#include <sys/arc.h>
#include <sys/zfs_vfsops.h>
#include <sys/zfs_ioctl_impl.h>

int
zfs_vfs_ref(zfsvfs_t **zfvp)
{
	/*
	 * Called with os_user_ptr_lock held. Do not wait for an unmount or
	 * suspension that may itself need this lock to clear the user pointer.
	 */
	if (*zfvp == NULL)
		return (SET_ERROR(ESRCH));
	if ((*zfvp)->z_vfs == NULL || vfs_trybusy((*zfvp)->z_vfs) != 0) {
		*zfvp = NULL;
		return (SET_ERROR(ESRCH));
	}
	return (0);
}

boolean_t
zfs_vfs_held(zfsvfs_t *zfsvfs)
{
	return (zfsvfs->z_vfs != NULL);
}

void
zfs_vfs_rele(zfsvfs_t *zfsvfs)
{
	vfs_t *mp = zfsvfs->z_vfs;
	boolean_t unmount;

	/*
	 * A failed resume (or receive of redacted/longname data) must remove
	 * the mount.  Wait until the ioctl has finished using zfsvfs: unmount
	 * frees it.  Take a native mount reference across the busy release.
	 */
	mutex_enter(&zfsvfs->z_lock);
	unmount = zfsvfs->z_unmount_pending;
	zfsvfs->z_unmount_pending = B_FALSE;
	if (unmount)
		vfs_ref(mp);
	mutex_exit(&zfsvfs->z_lock);
	vfs_unbusy(mp);
	if (unmount) {
		int error = dounmount(mp, MNT_FORCE, curlwp);
		if (error != 0)
			cmn_err(CE_WARN, "ZFS: cannot unmount %s after failed "
			    "resume: error %d", mp->mnt_stat.f_mntfromname,
			    error);
		vfs_rele(mp);
	}
}

void
zfs_ioctl_update_mount_cache(const char *dsname)
{
	zfsvfs_t *zfsvfs;

	if (getzfsvfs(dsname, &zfsvfs) == 0) {
		(void) VFS_STATVFS(zfsvfs->z_vfs, &zfsvfs->z_vfs->mnt_stat);
		zfs_vfs_rele(zfsvfs);
	}
}

uint64_t
zfs_max_nvlist_src_size_os(void)
{
	if (zfs_max_nvlist_src_size != 0)
		return (zfs_max_nvlist_src_size);
	return (arc_all_memory() / 4);
}

void
zfs_ioctl_init_os(void)
{
	/* There are no NetBSD-specific commands in the platform range yet. */
}

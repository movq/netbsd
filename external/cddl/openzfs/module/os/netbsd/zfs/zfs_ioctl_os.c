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
	vfs_unbusy(zfsvfs->z_vfs);
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

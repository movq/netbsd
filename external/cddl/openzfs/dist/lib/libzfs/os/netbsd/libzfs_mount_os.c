/* SPDX-License-Identifier: BSD-2-Clause */
#include <sys/mount.h>
#include <sys/mntent.h>
#include <libzfs.h>
#include "libzfs_impl.h"

int
do_mount(zfs_handle_t *zhp, const char *mntpt, const char *opts, int flags)
{
	struct zfs_args args = { 0 };
	struct mnttab mt = { .mnt_mntopts = __UNCONST(opts) };
	int native = flags & ~(MS_OVERLAY | MS_CRYPT);

	if (flags & MS_DETACH)
		return (ENOTSUP);
	if (strlcpy(args.fspec, zfs_get_name(zhp), sizeof (args.fspec)) >=
	    sizeof (args.fspec) ||
	    strlcpy(args.optptr, opts, sizeof (args.optptr)) >=
	    sizeof (args.optptr))
		return (ENAMETOOLONG);
	if (hasmntopt(&mt, MNTOPT_REMOUNT) != NULL)
		native |= MNT_UPDATE;
	if (hasmntopt(&mt, MNTOPT_RO) != NULL)
		native |= MNT_RDONLY;
	if (hasmntopt(&mt, MNTOPT_NOSUID) != NULL ||
	    hasmntopt(&mt, MNTOPT_NOSETUID) != NULL)
		native |= MNT_NOSUID;
	if (hasmntopt(&mt, MNTOPT_NODEVICES) != NULL)
		native |= MNT_NODEV;
	if (hasmntopt(&mt, MNTOPT_NOEXEC) != NULL)
		native |= MNT_NOEXEC;
	if (hasmntopt(&mt, MNTOPT_NOATIME) != NULL)
		native |= MNT_NOATIME;
	args.mflag = native;
	args.optlen = strlen(args.optptr) + 1;
	if (mount(MOUNT_ZFS, mntpt, native, &args, sizeof (args)) == -1)
		return (errno);
	return (0);
}

int
do_unmount(zfs_handle_t *zhp, const char *mntpt, int flags)
{
	(void) zhp;
	if (flags & MS_DETACH)
		return (ENOTSUP);
	flags &= ~(MS_CRYPT | MS_OVERLAY);
	return (unmount(mntpt, flags) == -1 ? errno : 0);
}

int
zfs_mount_setattr(zfs_handle_t *zhp, uint32_t flags)
{
	(void) flags;
	return (zfs_mount(zhp, MNTOPT_REMOUNT, 0));
}

int
zfs_mount_delegation_check(void)
{
	return (0);
}

void
zpool_disable_datasets_os(zpool_handle_t *zhp, boolean_t force)
{
	(void) zhp, (void) force;
}

void
zpool_disable_volume_os(const char *name)
{
	(void) name;
}

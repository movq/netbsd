/* SPDX-License-Identifier: BSD-2-Clause */
#include <sys/types.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <fcntl.h>
#include <libzfs.h>
#include <libzutil.h>
#include "libzfs_impl.h"

const char *
libzfs_error_init(int error)
{
	return (zfs_strerror(error));
}

int
libzfs_load_module(void)
{
	/* Retain osnet's module loading policy. Opening /dev/zfs follows. */
	modctl_load_t args = {
		.ml_filename = "zfs",
		.ml_flags = MODCTL_NO_PROP
	};
	if (modctl(MODCTL_LOAD, &args) == -1 && errno != EEXIST)
		return (errno);
	return (0);
}

int
zpool_label_disk(libzfs_handle_t *hdl, zpool_handle_t *zhp, const char *name)
{
	/* Devices are already partitioned, as in the old NetBSD integration. */
	(void) hdl, (void) zhp, (void) name;
	return (0);
}

int
zpool_relabel_disk(libzfs_handle_t *hdl, const char *path, const char *msg)
{
	(void) hdl, (void) path, (void) msg;
	return (0);
}

int
find_shares_object(differ_info_t *di)
{
	(void) di;
	return (0);
}

int
zfs_destroy_snaps_nvl_os(libzfs_handle_t *hdl, nvlist_t *snaps)
{
	/* No OS-specific work is needed before the common destroy ioctl. */
	(void) hdl, (void) snaps;
	return (0);
}

char *
zfs_version_kernel(void)
{
	size_t len;
	char *version;

	if (sysctlbyname("vfs.zfs.version.module", NULL, &len, NULL, 0) == -1)
		return (NULL);
	if ((version = malloc(len)) == NULL)
		return (NULL);
	if (sysctlbyname("vfs.zfs.version.module", version, &len, NULL, 0) == -1) {
		int error = errno;
		free(version);
		errno = error;
		return (NULL);
	}
	return (version);
}

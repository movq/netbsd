/* SPDX-License-Identifier: BSD-2-Clause */
#include <sys/stat.h>
#include <errno.h>
#include <paths.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libzutil.h>

/* NetBSD uses the selected disklabel partition or wedge without relabeling. */
char *
zfs_strip_partition(const char *path)
{
	return (strdup(path));
}

int
zfs_append_partition(char *path, size_t len)
{
	return (strnlen(path, len));
}

const char *
zfs_strip_path(const char *path)
{
	return (strncmp(path, _PATH_DEV, sizeof (_PATH_DEV) - 1) == 0 ?
	    path + sizeof (_PATH_DEV) - 1 : path);
}

char *
zfs_get_underlying_path(const char *path)
{
	return (path == NULL ? NULL : realpath(path, NULL));
}

boolean_t
zfs_dev_is_whole_disk(const char *path)
{
	/* As in osnet, disks and wedges are supplied already partitioned. */
	(void) path;
	return (B_FALSE);
}

boolean_t
is_mpath_whole_disk(const char *path)
{
	(void) path;
	return (B_FALSE);
}

int
zpool_label_disk_wait(const char *path, int timeout_ms)
{
	hrtime_t start = gethrtime();
	struct stat st;
	do {
		if (stat(path, &st) == 0)
			return (0);
		if (errno != ENOENT)
			return (errno);
		usleep(10000);
	} while (NSEC2MSEC(gethrtime() - start) < timeout_ms);
	return (ENODEV);
}

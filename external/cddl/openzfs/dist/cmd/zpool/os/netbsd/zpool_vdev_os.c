/* SPDX-License-Identifier: BSD-2-Clause */
#include <sys/types.h>
#include <errno.h>
#include <paths.h>
#include <stdio.h>
#include <string.h>
#include <libzfs.h>
#include "zpool_util.h"

int
check_device(const char *name, boolean_t force, boolean_t spare,
    boolean_t whole)
{
	char path[MAXPATHLEN];
	(void) whole;
	if (name[0] == '/')
		strlcpy(path, name, sizeof (path));
	else
		snprintf(path, sizeof (path), "%s%s", _PATH_DEV, name);
	return (check_file_generic(path, force, spare));
}

int
check_file(const char *path, boolean_t force, boolean_t spare)
{
	return (check_file_generic(path, force, spare));
}

boolean_t
check_sector_size_database(char *path, int *size)
{
	(void) path, (void) size;
	return (B_FALSE);
}

void
after_zpool_upgrade(zpool_handle_t *zhp)
{
	(void) zhp;
}

int
zpool_power_current_state(zpool_handle_t *zhp, char *vdev)
{
	(void) zhp, (void) vdev;
	return (-1);
}

int
zpool_power(zpool_handle_t *zhp, char *vdev, boolean_t on)
{
	(void) zhp, (void) vdev, (void) on;
	return (ENOTSUP);
}

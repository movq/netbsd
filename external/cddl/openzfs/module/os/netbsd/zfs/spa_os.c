/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * NetBSD pool hooks.  Pool activation and import need no additional
 * platform state, as in the previous NetBSD integration.
 */
#include <sys/zfs_context.h>
#include <sys/spa_impl.h>

const char *
spa_history_zone(void)
{
	return (NULL);
}

void
spa_import_os(spa_t *spa)
{
}

void
spa_export_os(spa_t *spa)
{
}

void
spa_activate_os(spa_t *spa)
{
}

void
spa_deactivate_os(spa_t *spa)
{
}

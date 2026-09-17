/* SPDX-License-Identifier: CDDL-1.0 */
/*
 * Copyright (c) 2012 by Delphix. All rights reserved.
 *
 * Bulk snapshot destruction without channel programs, adapted from the old
 * NetBSD port. Use the current single-snapshot checks and sync operations.
 */

#include <sys/zfs_context.h>
#include <sys/dmu_tx.h>
#include <sys/dsl_dir.h>
#include <sys/dsl_destroy.h>
#include <sys/dsl_synctask.h>
#include <sys/nvpair.h>

typedef struct netbsd_destroy_snaps_arg {
	nvlist_t *snaps;
	nvlist_t *errlist;
	boolean_t defer;
} netbsd_destroy_snaps_arg_t;

static int
netbsd_destroy_snaps_check(void *arg, dmu_tx_t *tx)
{
	netbsd_destroy_snaps_arg_t *args = arg;
	int error = 0;

	/* Check the entire batch under the sync task's config write lock. */
	if (!dmu_tx_is_syncing(tx))
		return (0);

	for (nvpair_t *pair = nvlist_next_nvpair(args->snaps, NULL);
	    pair != NULL; pair = nvlist_next_nvpair(args->snaps, pair)) {
		dsl_destroy_snapshot_arg_t snap = {
			.ddsa_name = nvpair_name(pair),
			.ddsa_defer = args->defer,
		};
		int err = dsl_destroy_snapshot_check(&snap, tx);

		/* The common check treats nonexistent snapshots as success. */
		if (err != 0) {
			fnvlist_add_int32(args->errlist, snap.ddsa_name, err);
			if (error == 0)
				error = err;
		}
	}
	return (error);
}

static void
netbsd_destroy_snaps_sync(void *arg, dmu_tx_t *tx)
{
	netbsd_destroy_snaps_arg_t *args = arg;

	for (nvpair_t *pair = nvlist_next_nvpair(args->snaps, NULL);
	    pair != NULL; pair = nvlist_next_nvpair(args->snaps, pair)) {
		dsl_destroy_snapshot_arg_t snap = {
			.ddsa_name = nvpair_name(pair),
			.ddsa_defer = args->defer,
		};

		/* Also removes zvol minors; nonexistent snapshots are ignored. */
		dsl_destroy_snapshot_sync(&snap, tx);
	}
}

int
dsl_destroy_snapshots_nvl(nvlist_t *snaps, boolean_t defer, nvlist_t *errlist)
{
	nvpair_t *first = nvlist_next_nvpair(snaps, NULL);
	netbsd_destroy_snaps_arg_t args = { snaps, errlist, defer };

	if (first == NULL)
		return (0);

	return (dsl_sync_task(nvpair_name(first), netbsd_destroy_snaps_check,
	    netbsd_destroy_snaps_sync, &args, 0, ZFS_SPACE_CHECK_DESTROY));
}

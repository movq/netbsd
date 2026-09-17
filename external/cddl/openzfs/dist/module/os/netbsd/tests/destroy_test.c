/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Exercise the real NetBSD batch adapter with a simulated sync task and
 * single-snapshot operations. This tests batching, not on-disk destruction.
 */
#include "destroy_shim.h"

static struct {
	const char *name;
	bool exists;
	int error;
	unsigned destroyed;
	unsigned deferred;
} snapshots[3];
static unsigned tasks, checks, syncs, deletes;
static int task_error;
static bool expected_defer;

nvpair_t *
nvlist_next_nvpair(nvlist_t *nvl, nvpair_t *prev)
{
	unsigned index = prev == NULL ? 0 : (unsigned)(prev - nvl->pairs) + 1;
	return (index < nvl->count ? &nvl->pairs[index] : NULL);
}

const char *
nvpair_name(nvpair_t *pair)
{
	return (pair->name);
}

void
fnvlist_add_int32(nvlist_t *nvl, const char *name, int32_t value)
{
	for (unsigned i = 0; i < nvl->count; i++) {
		if (strcmp(nvl->pairs[i].name, name) == 0) {
			nvl->pairs[i].value = value;
			return;
		}
	}
	assert(nvl->count < 16);
	nvl->pairs[nvl->count++] = (nvpair_t){ name, value };
}

bool
dmu_tx_is_syncing(dmu_tx_t *tx)
{
	return (tx->syncing);
}

static unsigned
snapshot_index(const char *name)
{
	for (unsigned i = 0; i < 3; i++)
		if (strcmp(snapshots[i].name, name) == 0)
			return (i);
	assert(!"unexpected snapshot");
	return (0);
}

int
dsl_destroy_snapshot_check(void *arg, dmu_tx_t *tx)
{
	dsl_destroy_snapshot_arg_t *snap = arg;
	unsigned i = snapshot_index(snap->ddsa_name);

	assert(tx->syncing);
	assert(snap->ddsa_defer == expected_defer);
	assert(deletes == 0);
	checks++;
	return (snapshots[i].exists ? snapshots[i].error : 0);
}

void
dsl_destroy_snapshot_sync(void *arg, dmu_tx_t *tx)
{
	dsl_destroy_snapshot_arg_t *snap = arg;
	unsigned i = snapshot_index(snap->ddsa_name);

	assert(tx->syncing);
	assert(snap->ddsa_defer == expected_defer);
	syncs++;
	if (!snapshots[i].exists)
		return;
	assert(snapshots[i].error == 0);
	deletes++;
	if (snap->ddsa_defer) {
		snapshots[i].deferred++;
	} else {
		snapshots[i].destroyed++;
		snapshots[i].exists = false;
	}
}

int
dsl_sync_task(const char *name, int (*check)(void *, dmu_tx_t *),
    void (*sync)(void *, dmu_tx_t *), void *arg, int blocks,
    zfs_space_check_t space)
{
	dmu_tx_t tx = { false };
	int error;

	assert(strcmp(name, snapshots[0].name) == 0);
	assert(blocks == 0);
	assert(space == ZFS_SPACE_CHECK_DESTROY);
	tasks++;
	/* Pool-open and space failures must propagate without destroying. */
	if (task_error != 0)
		return (task_error);
	assert(check(arg, &tx) == 0);
	assert(checks == 0 && syncs == 0);
	tx.syncing = true;
	error = check(arg, &tx);
	if (error == 0)
		sync(arg, &tx);
	return (error);
}

static void
reset(nvlist_t *snaps, nvlist_t *errors)
{
	memset(snapshots, 0, sizeof (snapshots));
	snapshots[0].name = "tank/fs@a";
	snapshots[1].name = "tank/fs@b";
	snapshots[2].name = "tank/fs@c";
	*snaps = (nvlist_t){ 0 };
	*errors = (nvlist_t){ 0 };
	for (unsigned i = 0; i < 3; i++) {
		snapshots[i].exists = true;
		/* Values are deliberately not booleans; the API ignores them. */
		fnvlist_add_int32(snaps, snapshots[i].name, -123);
	}
	tasks = checks = syncs = deletes = 0;
	task_error = 0;
	expected_defer = false;
}

int
main(void)
{
	nvlist_t snaps, errors;

	reset(&snaps, &errors);
	snaps.count = 0;
	assert(dsl_destroy_snapshots_nvl(&snaps, false, &errors) == 0);
	assert(tasks == 0 && errors.count == 0);

	reset(&snaps, &errors);
	assert(dsl_destroy_snapshots_nvl(&snaps, false, &errors) == 0);
	assert(tasks == 1 && checks == 3 && deletes == 3);
	assert(errors.count == 0);
	for (unsigned i = 0; i < 3; i++)
		assert(snapshots[i].destroyed == 1);

	/* A failure anywhere prevents all deletions and reports every error. */
	reset(&snaps, &errors);
	snapshots[1].error = EBUSY;
	snapshots[2].error = EEXIST;
	assert(dsl_destroy_snapshots_nvl(&snaps, false, &errors) == EBUSY);
	assert(checks == 3 && syncs == 0 && deletes == 0);
	assert(errors.count == 2);
	assert(strcmp(errors.pairs[0].name, snapshots[1].name) == 0);
	assert(errors.pairs[0].value == EBUSY);
	assert(strcmp(errors.pairs[1].name, snapshots[2].name) == 0);
	assert(errors.pairs[1].value == EEXIST);

	reset(&snaps, &errors);
	snapshots[0].exists = snapshots[2].exists = false;
	assert(dsl_destroy_snapshots_nvl(&snaps, false, &errors) == 0);
	assert(deletes == 1 && errors.count == 0);
	assert(snapshots[1].destroyed == 1);

	reset(&snaps, &errors);
	for (unsigned i = 0; i < 3; i++)
		snapshots[i].exists = false;
	assert(dsl_destroy_snapshots_nvl(&snaps, false, &errors) == 0);
	assert(deletes == 0 && errors.count == 0);

	reset(&snaps, &errors);
	expected_defer = true;
	assert(dsl_destroy_snapshots_nvl(&snaps, true, &errors) == 0);
	for (unsigned i = 0; i < 3; i++) {
		assert(snapshots[i].deferred == 1);
		assert(snapshots[i].destroyed == 0);
	}

	const int failures[] = { EIO, ENOSPC, EROFS };
	for (unsigned i = 0; i < 3; i++) {
		reset(&snaps, &errors);
		task_error = failures[i];
		assert(dsl_destroy_snapshots_nvl(&snaps, false, &errors) ==
		    task_error);
		assert(checks == 0 && syncs == 0 && errors.count == 0);
	}

	puts("snapshot batch checks passed");
	return (0);
}

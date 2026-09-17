/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef DESTROY_SHIM_H
#define DESTROY_SHIM_H

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef bool boolean_t;
typedef struct {
	const char *name;
	int32_t value;
} nvpair_t;
typedef struct {
	nvpair_t pairs[16];
	unsigned count;
} nvlist_t;
typedef struct {
	bool syncing;
} dmu_tx_t;
typedef struct {
	const char *ddsa_name;
	boolean_t ddsa_defer;
} dsl_destroy_snapshot_arg_t;
typedef enum { ZFS_SPACE_CHECK_DESTROY } zfs_space_check_t;

nvpair_t *nvlist_next_nvpair(nvlist_t *, nvpair_t *);
const char *nvpair_name(nvpair_t *);
void fnvlist_add_int32(nvlist_t *, const char *, int32_t);
bool dmu_tx_is_syncing(dmu_tx_t *);
int dsl_destroy_snapshot_check(void *, dmu_tx_t *);
void dsl_destroy_snapshot_sync(void *, dmu_tx_t *);
int dsl_sync_task(const char *, int (*)(void *, dmu_tx_t *),
    void (*)(void *, dmu_tx_t *), void *, int, zfs_space_check_t);
int dsl_destroy_snapshots_nvl(nvlist_t *, boolean_t, nvlist_t *);

#endif

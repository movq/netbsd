/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef VFS_POLICY_SHIM_H
#define VFS_POLICY_SHIM_H
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef bool boolean_t;
#define B_TRUE true
#define B_FALSE false
#define SET_ERROR(e) (e)
#define FTAG "test"
#define SPA_FEATURE_LONGNAME 1
#define ZFS_PROP_LONGNAME 1
#define ZPROP_SRC_LOCAL 1
#define ZPROP_SRC_RECEIVED 2
#define ZPROP_SRC_NONE 4
#define MNT_FORCE 1
#define CE_WARN 1
#define CE_NOTE 2
#define curlwp NULL

typedef struct dsl_dir { bool dd_activity_cancelled; } dsl_dir_t;
typedef struct dsl_dataset {
	bool longname_active;
	dsl_dir_t *ds_dir;
} dsl_dataset_t;
typedef struct objset {
	dsl_dataset_t *ds;
	bool *os_spa;
} objset_t;
typedef struct mount {
	unsigned busy, refs;
	bool unmounted;
	struct { char f_mntfromname[32]; } mnt_stat;
	void *fs;
} vfs_t;
typedef struct znode {
	struct znode *next;
	bool rebound;
} znode_t;
typedef struct zfsvfs {
	objset_t *z_os;
	vfs_t *z_vfs;
	bool z_unmount_pending;
	int z_lock, z_znodes_lock;
	znode_t *z_all_znodes;
} zfsvfs_t;

static inline void mutex_enter(int *lock) { assert(!*lock); *lock = 1; }
static inline void mutex_exit(int *lock) { assert(*lock); *lock = 0; }
static inline znode_t *list_head(znode_t **head) { return *head; }
static inline znode_t *list_next(znode_t **head, znode_t *node)
{ return node->next; }
static inline dsl_dataset_t *dmu_objset_ds(objset_t *os) { return os->ds; }
static inline bool dsl_dataset_feature_is_active(dsl_dataset_t *ds, int f)
{ assert(f == SPA_FEATURE_LONGNAME); return ds->longname_active; }
static inline bool spa_feature_is_enabled(bool *spa, int f) { return *spa; }

void cmn_err(int, const char *, ...);
objset_t *zfs_suspended_objset(zfsvfs_t *, dsl_dataset_t *);
int zfsvfs_init(zfsvfs_t *, objset_t *);
int zfsvfs_setup(zfsvfs_t *, bool);
int zfs_rezget(znode_t *);
void zfs_resume_finish(zfsvfs_t *, bool);
int zfsvfs_hold(const char *, const char *, zfsvfs_t **, bool);
void zfsvfs_rele(zfsvfs_t *, const char *);
void vfs_ref(vfs_t *);
void vfs_rele(vfs_t *);
void vfs_unbusy(vfs_t *);
int dounmount(vfs_t *, int, void *);
int zfs_netbsd_check_mount(objset_t *);
int zfs_resume_fs(zfsvfs_t *, dsl_dataset_t *);
void zfs_vfs_rele(zfsvfs_t *);
int set_longname(int, uint64_t);
#endif

/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef FID_SHIM_H
#define FID_SHIM_H
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "fid_format.h"
typedef bool boolean_t;
#define B_FALSE false
#define B_TRUE true
#define FTAG "test"
#define SET_ERROR(e) (e)
#define ZFSCTL_INO_ROOT 1
#define ZFSCTL_INO_SNAPDIR 2
#define SA_ZPL_GEN(fs) 1
typedef struct { uint16_t fid_len; } fid_t;
typedef struct { uint64_t id; } objset_t;
typedef struct znode znode_t;
typedef struct vnode { znode_t *node; unsigned refs; bool locked; } vnode_t;
typedef struct zfsvfs {
	struct zfsvfs *z_parent;
	objset_t *z_os;
	unsigned entered, busy;
} zfsvfs_t;
typedef struct { zfsvfs_t *mnt_data; } vfs_t;
struct znode {
	zfsvfs_t *z_zfsvfs;
	vnode_t *vp;
	uint64_t z_id;
	uint64_t *z_sa_hdl;
	bool z_unlinked;
};
#define VTOZ(vp) ((vp)->node)
#define ZTOV(zp) ((zp)->vp)
int zfs_enter(zfsvfs_t *, const char *);
int zfs_enter_verify_zp(zfsvfs_t *, znode_t *, const char *);
void zfs_exit(zfsvfs_t *, const char *);
int sa_lookup(uint64_t *, int, void *, size_t);
static inline uint64_t dmu_objset_id(objset_t *os) { return os->id; }
static inline bool zfsctl_is_node(vnode_t *vp) { return false; }
int zfsctl_vptofh(vnode_t *, fid_t *, size_t *);
int zfsctl_lookup_objset(vfs_t *, uint64_t, zfsvfs_t **);
int zfsctl_root(zfsvfs_t *, vnode_t **);
int zfsctl_snapshot(zfsvfs_t *, vnode_t **);
int zfs_zget(zfsvfs_t *, uint64_t, znode_t **);
void zfs_vfs_rele(zfsvfs_t *);
void vrele(vnode_t *);
int vn_lock(vnode_t *, int);
int zfs_netbsd_vptofh(vnode_t *, fid_t *, size_t *);
int zfs_netbsd_fhtovp(vfs_t *, fid_t *, int, vnode_t **);
#endif

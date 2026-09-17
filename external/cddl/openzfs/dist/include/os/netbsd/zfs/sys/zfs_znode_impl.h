/* SPDX-License-Identifier: CDDL-1.0 */
/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2015 by Delphix. All rights reserved.
 * Copyright (c) 2014 Integros [integros.com]
 * Copyright 2016 Nexenta Systems, Inc. All rights reserved.
 */
#ifndef _NETBSD_ZFS_ZNODE_IMPL_H_
#define	_NETBSD_ZFS_ZNODE_IMPL_H_

#include <sys/list.h>
#include <sys/dmu.h>
#include <sys/sa.h>
#include <sys/zfs_vfsops.h>
#include <sys/zfs_sa.h>
#include <sys/zfs_stat.h>
#include <sys/zfs_rlock.h>
#include <sys/zfs_acl.h>
#include <sys/zil.h>
#include <sys/zfs_project.h>
#include <sys/uio.h>
#include <miscfs/genfs/genfs_node.h>

#define	ZNODE_OS_FIELDS			\
	struct zfsvfs	*z_zfsvfs;	\
	vnode_t		*z_vnode;	\
	uint64_t	z_uid;		\
	uint64_t	z_gid;		\
	uint64_t	z_gen;		\
	uint64_t	z_atime[2];	\
	uint64_t	z_links;	\
	struct lockf	*z_lockf

#define	ZFS_LINK_MAX	UINT64_MAX

enum zfs_soft_state_type {
	ZSST_ZVOL,
	ZSST_CTLDEV
};

typedef struct zfs_soft_state {
	enum zfs_soft_state_type zss_type;
	void *zss_data;
} zfs_soft_state_t;

#define	ZTOV(zp)	((zp)->z_vnode)
#define	ZTOI(zp)	ZTOV(zp)
#define	VTOZ(vp)	((struct znode *)(vp)->v_data)
#define	ITOZ(vp)	VTOZ(vp)
#define	zhold(zp)	vhold(ZTOV(zp))
#define	zrele(zp)	vrele(ZTOV(zp))
#define	ZTOZSB(zp)	((zp)->z_zfsvfs)
#define	ITOZSB(vp)	(VTOZ(vp)->z_zfsvfs)
#define	ZTOTYPE(zp)	(ZTOV(zp)->v_type)
#define	ZTOGID(zp)	((zp)->z_gid)
#define	ZTOUID(zp)	((zp)->z_uid)
#define	ZTONLNK(zp)	((zp)->z_links)
#define	Z_ISBLK(type)	((type) == VBLK)
#define	Z_ISCHR(type)	((type) == VCHR)
#define	Z_ISLNK(type)	((type) == VLNK)
#define	Z_ISDIR(type)	((type) == VDIR)

#define	zn_has_cached_data(zp, start, end)	vn_has_cached_data(ZTOV(zp))
#define	zn_flush_cached_data(zp, sync)	zfs_flush_cached_data(ZTOV(zp), sync)
#define	zn_rlimit_fsize(size)	zfs_rlimit_fsize(size)
#define	zn_rlimit_fsize_uio(zp, uio)	\
	zfs_rlimit_fsize_uio(ZTOV(zp), uio)

extern void zfs_flush_cached_data(vnode_t *, boolean_t);
extern int zfs_rlimit_fsize(uint64_t);
extern int zfs_rlimit_fsize_uio(vnode_t *, zfs_uio_t *);

static inline int
zfs_enter(zfsvfs_t *zfsvfs, const char *tag)
{
	ZFS_TEARDOWN_ENTER_READ(zfsvfs, tag);
	if (__predict_false(zfsvfs->z_unmounted)) {
		ZFS_TEARDOWN_EXIT_READ(zfsvfs, tag);
		return (SET_ERROR(EIO));
	}
	return (0);
}

static inline void
zfs_exit(zfsvfs_t *zfsvfs, const char *tag)
{
	ZFS_TEARDOWN_EXIT_READ(zfsvfs, tag);
}

#define	ZFS_OBJ_HASH(obj)	((obj) & (ZFS_OBJ_MTX_SZ - 1))
#define	ZFS_OBJ_MUTEX(fs, obj)	(&(fs)->z_hold_mtx[ZFS_OBJ_HASH(obj)])
#define	ZFS_OBJ_HOLD_ENTER(fs, obj)	mutex_enter(ZFS_OBJ_MUTEX(fs, obj))
#define	ZFS_OBJ_HOLD_TRYENTER(fs, obj)	mutex_tryenter(ZFS_OBJ_MUTEX(fs, obj))
#define	ZFS_OBJ_HOLD_EXIT(fs, obj)	mutex_exit(ZFS_OBJ_MUTEX(fs, obj))

#define	ZFS_TIME_ENCODE(tp, stmp) do {		\
	(stmp)[0] = (uint64_t)(tp)->tv_sec;	\
	(stmp)[1] = (uint64_t)(tp)->tv_nsec;	\
} while (0)
#define	ZFS_TIME_DECODE(tp, stmp) do {		\
	(tp)->tv_sec = (time_t)(stmp)[0];		\
	(tp)->tv_nsec = (long)(stmp)[1];		\
} while (0)

#define	ZFS_ACCESSTIME_STAMP(fs, zp) do {				\
	if ((fs)->z_atime && !((fs)->z_vfs->vfs_flag & VFS_RDONLY))	\
		zfs_tstamp_update_setup_ext(zp, ACCESSED, NULL, NULL, B_FALSE); \
} while (0)

extern void zfs_tstamp_update_setup_ext(struct znode *, uint_t,
    uint64_t [2], uint64_t [2], boolean_t);
extern void zfs_znode_free(struct znode *);
extern zil_replay_func_t *const zfs_replay_vector[TX_MAX_TYPE];
extern int zfs_znode_parent_and_name(struct znode *, struct znode **,
    char *, uint64_t);

#endif

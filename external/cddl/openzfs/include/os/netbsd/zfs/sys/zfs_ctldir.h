/* SPDX-License-Identifier: CDDL-1.0 */
/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 */
#ifndef _NETBSD_ZFS_CTLDIR_H_
#define	_NETBSD_ZFS_CTLDIR_H_

#include <sys/vnode.h>
#include <sys/zfs_vfsops.h>
#include <sys/zfs_znode.h>

#define	ZFS_CTLDIR_NAME	".zfs"
#define	zfs_has_ctldir(zdp)	\
	((zdp)->z_id == (zdp)->z_zfsvfs->z_root && \
	(zdp)->z_zfsvfs->z_ctldir != NULL)
#define	zfs_show_ctldir(zdp)	\
	(zfs_has_ctldir(zdp) && \
	(zdp)->z_zfsvfs->z_show_ctldir == ZFS_SNAPDIR_VISIBLE)

void zfsctl_create(zfsvfs_t *);
void zfsctl_destroy(zfsvfs_t *);
int zfsctl_loadvnode(vfs_t *, vnode_t *, const void *, size_t, const void **);
int zfsctl_vptofh(vnode_t *, fid_t *, size_t *);
int zfsctl_root(zfsvfs_t *, vnode_t **);
int zfsctl_snapshot(zfsvfs_t *, vnode_t **);
void zfsctl_init(void);
void zfsctl_fini(void);
boolean_t zfsctl_is_node(vnode_t *);
int zfsctl_snapshot_unmount(const char *, int);
int zfsctl_rename_snapshot(const char *, const char *);
int zfsctl_destroy_snapshot(const char *, int);
int zfsctl_umount_snapshots(vfs_t *, int, cred_t *);
int zfsctl_lookup_objset(vfs_t *, uint64_t, zfsvfs_t **);

#define	ZFSCTL_INO_ROOT		0x1
#define	ZFSCTL_INO_SNAPDIR	0x2

#endif

/* SPDX-License-Identifier: CDDL-1.0 */
/*
 * NetBSD vnode construction interface for the shared ZFS metadata code.
 */
#ifndef _NETBSD_ZFS_VCACHE_H_
#define	_NETBSD_ZFS_VCACHE_H_

#include <sys/zfs_znode.h>

extern int (**zfs_vnodeop_p)(void *);
extern int (**zfs_specop_p)(void *);
extern int (**zfs_fifoop_p)(void *);
extern const struct genfs_ops zfs_genfsops;

znode_t *zfs_znode_alloc(zfsvfs_t *, dmu_buf_t *, int, dmu_object_type_t,
    sa_handle_t *, vnode_t *);
void zfs_mknode_impl(znode_t *, vattr_t *, dmu_tx_t *, cred_t *, uint_t,
    znode_t **, zfs_acl_ids_t *, vnode_t *);
int zfs_loadvnode(struct mount *, vnode_t *, const void *, size_t,
    const void **);
int zfs_newvnode(struct mount *, vnode_t *, vnode_t *, vattr_t *, cred_t *,
    void *, size_t *, const void **);
void zfs_netbsd_setsize(vnode_t *, uint64_t);

#endif

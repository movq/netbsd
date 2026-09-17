/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_ZFS_VNOPS_OS_H_
#define	_NETBSD_ZFS_VNOPS_OS_H_

extern int zfs_remove(znode_t *, const char *, cred_t *, int);
extern int zfs_mkdir(znode_t *, const char *, vattr_t *, znode_t **,
    cred_t *, int, vsecattr_t *, zidmap_t *);
extern int zfs_rmdir(znode_t *, const char *, znode_t *, cred_t *, int);
extern int zfs_setattr(znode_t *, vattr_t *, int, cred_t *, zidmap_t *);
extern int zfs_rename(znode_t *, const char *, znode_t *, const char *,
    cred_t *, int, uint64_t, vattr_t *, zidmap_t *);
extern int zfs_symlink(znode_t *, const char *, vattr_t *, const char *,
    znode_t **, cred_t *, int, zidmap_t *);
extern int zfs_link(znode_t *, znode_t *, const char *, cred_t *, int);
extern int zfs_space(znode_t *, int, struct flock *, int, offset_t, cred_t *);
extern int zfs_create(znode_t *, const char *, vattr_t *, int, int, znode_t **,
    cred_t *, int, vsecattr_t *, zidmap_t *);
extern int zfs_write_simple(znode_t *, const void *, size_t, loff_t, size_t *);

#endif

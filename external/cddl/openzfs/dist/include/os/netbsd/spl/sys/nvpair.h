/* $NetBSD$ */

#ifndef _NETBSD_SPL_SYS_NVPAIR_H_
#define _NETBSD_SPL_SYS_NVPAIR_H_

/*
 * NetBSD's kernel libnv exports these names with an incompatible ABI.
 * Keep OpenZFS's nvpair implementation private to the ZFS module.
 */
#define nvlist_add_nvlist	zfs_nvlist_add_nvlist
#define nvlist_add_nvlist_array	zfs_nvlist_add_nvlist_array
#define nvlist_add_nvpair	zfs_nvlist_add_nvpair
#define nvlist_add_string	zfs_nvlist_add_string
#define nvlist_add_string_array	zfs_nvlist_add_string_array
#define nvlist_empty		zfs_nvlist_empty
#define nvlist_exists		zfs_nvlist_exists
#define nvlist_free		zfs_nvlist_free
#define nvlist_next_nvpair	zfs_nvlist_next_nvpair
#define nvlist_pack		zfs_nvlist_pack
#define nvlist_prev_nvpair	zfs_nvlist_prev_nvpair
#define nvlist_remove_nvpair	zfs_nvlist_remove_nvpair
#define nvlist_size		zfs_nvlist_size
#define nvlist_unpack		zfs_nvlist_unpack
#define nvpair_name		zfs_nvpair_name
#define nvpair_type		zfs_nvpair_type

/* The retained Solaris module also provides the old nvpair allocators. */
#define nv_alloc_init		zfs_nv_alloc_init
#define nv_alloc_fini		zfs_nv_alloc_fini
#define nv_alloc_reset		zfs_nv_alloc_reset
#define nv_alloc_nosleep		zfs_nv_alloc_nosleep
#define nv_alloc_sleep		zfs_nv_alloc_sleep
#define nv_fixed_ops		zfs_nv_fixed_ops

#include_next <sys/nvpair.h>

#endif /* _NETBSD_SPL_SYS_NVPAIR_H_ */

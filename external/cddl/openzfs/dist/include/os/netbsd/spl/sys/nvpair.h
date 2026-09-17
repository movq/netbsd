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

#include_next <sys/nvpair.h>

#endif /* _NETBSD_SPL_SYS_NVPAIR_H_ */

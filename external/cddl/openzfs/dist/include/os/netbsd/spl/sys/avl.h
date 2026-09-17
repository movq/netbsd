/* $NetBSD$ */

#ifndef _NETBSD_SPL_SYS_AVL_H_
#define _NETBSD_SPL_SYS_AVL_H_

/*
 * OpenZFS's kernel avl_tree_t no longer contains avl_size.  The Solaris
 * module's implementation uses the old layout and must not service these
 * calls.
 */
#define avl_add			zfs_avl_add
#define avl_create		zfs_avl_create
#define avl_destroy		zfs_avl_destroy
#define avl_destroy_nodes	zfs_avl_destroy_nodes
#define avl_find			zfs_avl_find
#define avl_first		zfs_avl_first
#define avl_insert		zfs_avl_insert
#define avl_insert_here		zfs_avl_insert_here
#define avl_is_empty		zfs_avl_is_empty
#define avl_last			zfs_avl_last
#define avl_nearest		zfs_avl_nearest
#define avl_numnodes		zfs_avl_numnodes
#define avl_remove		zfs_avl_remove
#define avl_swap			zfs_avl_swap
#define avl_update		zfs_avl_update
#define avl_update_gt		zfs_avl_update_gt
#define avl_update_lt		zfs_avl_update_lt
#define avl_walk			zfs_avl_walk

#include_next <sys/avl.h>

#endif /* _NETBSD_SPL_SYS_AVL_H_ */

/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_FCNTL_H_
#define	_NETBSD_SPL_FCNTL_H_

#include_next <sys/fcntl.h>

/* Native file offsets are always 64-bit. */
#define	O_LARGEFILE	0

typedef struct flock flock64_t;
#define	F_FREESP	11	/* Internal zfs_space operation. */

#endif

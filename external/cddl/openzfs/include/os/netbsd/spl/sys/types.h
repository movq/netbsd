/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_TYPES_H_
#define	_NETBSD_SPL_TYPES_H_

#include_next <sys/types.h>

typedef off_t		loff_t;
typedef struct timespec	inode_timespec_t;

#endif

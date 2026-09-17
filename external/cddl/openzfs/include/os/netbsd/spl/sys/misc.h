/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_MISC_H_
#define	_NETBSD_SPL_MISC_H_

#include_next <sys/misc.h>

/* The current common zvol header supplies its own device offset limit. */
#undef SPEC_MAXOFFSET_T

#define	current_is_reclaim_thread	openzfs_current_is_reclaim_thread
int current_is_reclaim_thread(void);

#endif

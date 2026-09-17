/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_RWLOCK_H_
#define	_NETBSD_SPL_RWLOCK_H_

#include_next <sys/rwlock.h>

/* Sentinel for interfaces which optionally acquire a lock. */
#define	RW_NONE	((krw_t)2)

#endif

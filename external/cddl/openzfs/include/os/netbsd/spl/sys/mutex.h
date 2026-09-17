/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_MUTEX_H_
#define	_NETBSD_SPL_MUTEX_H_

#include_next <sys/mutex.h>

/* Linux lockdep subclasses do not change native locking semantics. */
#define	mutex_enter_nested(lock, subclass)	mutex_enter(lock)

#define	mutex_enter_interruptible	openzfs_mutex_enter_interruptible
int mutex_enter_interruptible(kmutex_t *);

#endif

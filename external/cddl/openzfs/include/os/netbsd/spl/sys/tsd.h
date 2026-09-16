/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_TSD_H_
#define	_NETBSD_SPL_TSD_H_

#include <sys/lwp.h>
#include <sys/debug.h>

/* Use the same LWP-specific storage as the old NetBSD ZFS integration. */
static inline void
tsd_create(uint_t *key, void (*destructor)(void *))
{
	VERIFY0(lwp_specific_key_create(key, destructor));
}

static inline void
tsd_destroy(uint_t *key)
{
	lwp_specific_key_delete(*key);
}

static inline void *
tsd_get(uint_t key)
{
	return (lwp_getspecific(key));
}

static inline int
tsd_set(uint_t key, void *value)
{
	lwp_setspecific(key, value);
	return (0);
}

#endif

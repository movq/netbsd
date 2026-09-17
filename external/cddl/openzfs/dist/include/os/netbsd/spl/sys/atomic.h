/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_ATOMIC_H_
#define	_NETBSD_SPL_ATOMIC_H_

#include_next <sys/atomic.h>

static inline void
atomic_sub_64(volatile uint64_t *p, uint64_t value)
{
	/* Unsigned negation also handles values greater than INT64_MAX. */
	atomic_add_64(p, (int64_t)(0 - value));
}

/*
 * OpenZFS's plain atomic loads/stores do not imply a memory barrier.
 * Use the native accessors so NetBSD's race instrumentation sees them.
 * On 32-bit machines use atomic RMW operations to avoid torn 64-bit access.
 */
static inline uint64_t
atomic_load_64(volatile uint64_t *p)
{
#ifdef __HAVE_ATOMIC64_LOADSTORE
	return (atomic_load_relaxed(p));
#else
	return (atomic_cas_64(p, 0, 0));
#endif
}

static inline void
atomic_store_64(volatile uint64_t *p, uint64_t value)
{
#ifdef __HAVE_ATOMIC64_LOADSTORE
	atomic_store_relaxed(p, value);
#else
	(void)atomic_swap_64(p, value);
#endif
}

#endif

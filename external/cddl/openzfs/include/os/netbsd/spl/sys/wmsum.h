/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_WMSUM_H_
#define	_NETBSD_SPL_WMSUM_H_

#include <sys/types.h>
#include <sys/atomic.h>

/*
 * A single atomic counter provides the required statistics semantics.
 * Per-CPU aggregation can replace this if contention warrants it.
 */
typedef struct {
	volatile uint64_t value;
} wmsum_t;

static inline void
wmsum_init(wmsum_t *sum, uint64_t value)
{
	sum->value = value;
}

static inline void
wmsum_fini(wmsum_t *sum)
{
}

static inline uint64_t
wmsum_value(wmsum_t *sum)
{
	return (atomic_load_64(&sum->value));
}

static inline void
wmsum_add(wmsum_t *sum, int64_t delta)
{
	atomic_add_64(&sum->value, delta);
}

#endif

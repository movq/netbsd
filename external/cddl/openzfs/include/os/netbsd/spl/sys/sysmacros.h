/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_SYSMACROS_H_
#define	_NETBSD_SPL_SYSMACROS_H_

#include_next <sys/sysmacros.h>

#define	ARRAY_SIZE(a)	__arraycount(a)
#define	DIV_ROUND_UP(n, d)	howmany((n), (d))
#define	____cacheline_aligned	__aligned(COHERENCY_UNIT)
#define	boot_ncpus	ncpu

/*
 * The old wrapper uses NetBSD's fls64(), but OpenZFS's sys/bitops.h shadows
 * the native header. Keep the one-based result, including zero for zero.
 */
#undef	highbit
#undef	highbit64
static inline int
highbit(ulong_t value)
{
	return (value == 0 ? 0 :
	    sizeof (value) * NBBY - __builtin_clzl(value));
}

static inline int
highbit64(uint64_t value)
{
	return (value == 0 ? 0 : 64 - __builtin_clzll(value));
}

#endif

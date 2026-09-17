/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_DEBUG_H_
#define	_NETBSD_SPL_DEBUG_H_

#include_next <sys/debug.h>

#define	PANIC(...)	panic(__VA_ARGS__)

#define	ASSERT0P(p)	ASSERT3P((p), ==, NULL)
#define	VERIFY0P(p)	VERIFY3P((p), ==, NULL)
#define	ASSERT3B(a, op, b)	ASSERT3U(!!(a), op, !!(b))
#define	VERIFY3B(a, op, b)	VERIFY3U(!!(a), op, !!(b))

/* Retain the invariant checks; native panic lacks OpenZFS's %px format. */
#define	ASSERTF(expr, fmt, ...)	ASSERT(expr)
#define	ASSERT3PF(a, op, b, fmt, ...)	ASSERT3P(a, op, b)
#define	ASSERT3UF(a, op, b, fmt, ...)	ASSERT3U(a, op, b)
#define	ASSERT3SF(a, op, b, fmt, ...)	ASSERT3S(a, op, b)

#endif

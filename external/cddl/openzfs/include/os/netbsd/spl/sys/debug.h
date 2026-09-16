/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_DEBUG_H_
#define	_NETBSD_SPL_DEBUG_H_

#include_next <sys/debug.h>

#define	ASSERT0P(p)	ASSERT3P((p), ==, NULL)
#define	VERIFY0P(p)	VERIFY3P((p), ==, NULL)
#define	ASSERT3B(a, op, b)	ASSERT3U(!!(a), op, !!(b))
#define	VERIFY3B(a, op, b)	VERIFY3U(!!(a), op, !!(b))

#endif

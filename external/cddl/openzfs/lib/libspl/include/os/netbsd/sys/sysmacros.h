/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _LIBSPL_NETBSD_SYSMACROS_H_
#define	_LIBSPL_NETBSD_SYSMACROS_H_
#include <sys/param.h>

#define	ARRAY_SIZE(a)	__arraycount(a)
#define	DIV_ROUND_UP(n, d)	howmany(n, d)
#define	ABS(a)		((a) < 0 ? -(a) : (a))
#define	makedevice(maj, min)	makedev(maj, min)
#define	_sysconf(a)	sysconf(a)
#define	ISP2(x)		(((x) & ((x) - 1)) == 0)
#define	IS_P2ALIGNED(v, a)	\
	((((uintptr_t)(v)) & ((uintptr_t)(a) - 1)) == 0)
#define	P2CROSS(x, y, a)	(((x) ^ (y)) > (a) - 1)
#define	P2ROUNDUP(x, a)	((((x) - 1) | ((a) - 1)) + 1)
#define	P2BOUNDARY(o, l, a)	(((o) ^ ((o) + (l) - 1)) > (a) - 1)
#define	P2PHASE(x, a)	((x) & ((a) - 1))
#define	P2NPHASE(x, a)	(-(x) & ((a) - 1))
#define	P2ALIGN_TYPED(x, a, t)	((t)(x) & -(t)(a))
#define	P2PHASE_TYPED(x, a, t)	((t)(x) & ((t)(a) - 1))
#define	P2NPHASE_TYPED(x, a, t)	(-(t)(x) & ((t)(a) - 1))
#define	P2ROUNDUP_TYPED(x, a, t)	((((t)(x) - 1) | ((t)(a) - 1)) + 1)
#define	P2END_TYPED(x, a, t)	(-(~(t)(x) & -(t)(a)))
#define	P2PHASEUP_TYPED(x, a, p, t)	\
	((t)(p) - (((t)(p) - (t)(x)) & -(t)(a)))
#define	P2CROSS_TYPED(x, y, a, t)	(((t)(x) ^ (t)(y)) > (t)(a) - 1)
#define	P2SAMEHIGHBIT_TYPED(x, y, t)	\
	(((t)(x) ^ (t)(y)) < ((t)(x) & (t)(y)))
#endif

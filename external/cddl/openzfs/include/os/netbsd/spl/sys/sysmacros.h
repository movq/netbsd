/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_SYSMACROS_H_
#define	_NETBSD_SPL_SYSMACROS_H_

#include_next <sys/sysmacros.h>

#define	ARRAY_SIZE(a)	__arraycount(a)

#endif

/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_TYPES32_H_
#define	_NETBSD_SPL_TYPES32_H_

#include <sys/types.h>

/* Solaris fixed-width types, not the NetBSD compat32 syscall ABI. */
typedef uint32_t	caddr32_t;
typedef int32_t	daddr32_t;
typedef int32_t	time32_t;
typedef uint32_t	size32_t;

#endif

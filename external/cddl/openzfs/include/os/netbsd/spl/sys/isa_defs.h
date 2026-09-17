/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_ISA_DEFS_H_
#define	_NETBSD_SPL_ISA_DEFS_H_

#include_next <sys/isa_defs.h>
#include <sys/endian.h>

#if BYTE_ORDER == LITTLE_ENDIAN
#define	_ZFS_LITTLE_ENDIAN
#elif BYTE_ORDER == BIG_ENDIAN
#define	_ZFS_BIG_ENDIAN
#else
#error "Unknown NetBSD byte order"
#endif

#endif

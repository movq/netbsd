/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_ERRNO_H_
#define	_NETBSD_SPL_ERRNO_H_

#include_next <sys/errno.h>

/* Preserve the old NetBSD ZFS port's internal checksum error number. */
#define	ECKSUM	122
#define	EREMOTEIO	EREMOTE
#define	ENOTACTIVE	ECANCELED

#endif

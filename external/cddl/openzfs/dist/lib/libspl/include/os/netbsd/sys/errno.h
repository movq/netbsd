/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _LIBSPL_NETBSD_ERRNO_H_
#define	_LIBSPL_NETBSD_ERRNO_H_
#include_next <sys/errno.h>
/* Keep these synchronized with include/os/netbsd/spl/sys/errno.h. */
#define	ECKSUM	122
#define	EREMOTEIO	EREMOTE
#define	ENOTACTIVE	ECANCELED
#define	ECHRNG	ENXIO
#endif

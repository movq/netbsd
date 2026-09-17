/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _LIBSPL_NETBSD_FCNTL_H_
#define	_LIBSPL_NETBSD_FCNTL_H_
#include_next <fcntl.h>
#define	open64	open
#define	openat64	openat
#define	O_LARGEFILE	0
#endif

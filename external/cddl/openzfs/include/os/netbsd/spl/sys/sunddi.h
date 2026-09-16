/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_SUNDDI_H_
#define	_NETBSD_SPL_SUNDDI_H_

#include <sys/systm.h>

/*
 * These conversions are supplied by the existing NetBSD Solaris layer.
 * Do not include its sunddi.h: its sysevent interface uses the old event
 * representation, which differs from OpenZFS's nvlist-based representation.
 */
int ddi_strtol(const char *, char **, int, long *);
int ddi_strtoul(const char *, char **, int, unsigned long *);
int ddi_strtoull(const char *, char **, int, unsigned long long *);

#define	ddi_copyin(from, to, size, flag) \
	ioctl_copyin((flag), (from), (to), (size))
#define	ddi_copyout(from, to, size, flag) \
	ioctl_copyout((flag), (from), (to), (size))

#endif

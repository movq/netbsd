/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_TIME_H_
#define	_NETBSD_SPL_TIME_H_

#include_next <sys/time.h>

#define	USEC2NSEC(us)	((hrtime_t)(us) * (NANOSEC / MICROSEC))
#define	NSEC2USEC(ns)	((ns) / (NANOSEC / MICROSEC))
#define	getlrtime()	gethrtime()

#endif

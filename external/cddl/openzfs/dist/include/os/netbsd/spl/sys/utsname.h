/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_UTSNAME_H_
#define	_NETBSD_SPL_UTSNAME_H_

#include_next <sys/utsname.h>

/* The shared Solaris module initializes this native utsname snapshot. */
typedef struct utsname utsname_t;
extern struct utsname utsname;
#define	utsname()	(&utsname)

#endif

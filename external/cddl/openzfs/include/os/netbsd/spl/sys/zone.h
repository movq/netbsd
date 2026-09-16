/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_ZONE_H_
#define	_NETBSD_SPL_ZONE_H_

#include_next <sys/zone.h>

#ifndef GLOBAL_ZONEID
#define	GLOBAL_ZONEID	0
#endif
#define	crgetzoneid(cr)	GLOBAL_ZONEID

#endif

/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _LIBSPL_NETBSD_SYS_PARAM_H_
#define	_LIBSPL_NETBSD_SYS_PARAM_H_
#include_next <sys/param.h>
#include <unistd.h>

#define	MAXNAMELEN	256
#define	UID_NOACCESS	60002
#define	MAXUID		UINT32_MAX
#define	MAXPROJID	MAXUID

extern size_t spl_pagesize(void);
#undef	PAGESIZE
#define	PAGESIZE	(spl_pagesize())
#undef	ptob
#endif

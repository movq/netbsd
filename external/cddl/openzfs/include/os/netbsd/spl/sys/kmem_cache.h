/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_KMEM_CACHE_H_
#define	_NETBSD_SPL_KMEM_CACHE_H_

/* The NetBSD Solaris layer declares both allocation APIs in kmem.h. */
#include <sys/kmem.h>

/* NetBSD pool caches do not move objects; set_move remains a no-op. */
typedef enum kmem_cbrc {
	KMEM_CBRC_YES,
	KMEM_CBRC_NO,
	KMEM_CBRC_LATER,
	KMEM_CBRC_DONT_NEED,
	KMEM_CBRC_DONT_KNOW
} kmem_cbrc_t;

#endif

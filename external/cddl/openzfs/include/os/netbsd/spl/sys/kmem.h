/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_KMEM_H_
#define	_NETBSD_SPL_KMEM_H_

#include_next <sys/kmem.h>

/* NetBSD pool caches already participate in memory reclamation. */
#define	KMC_RECLAIMABLE	0

#endif

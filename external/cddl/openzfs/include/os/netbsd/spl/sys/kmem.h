/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_KMEM_H_
#define	_NETBSD_SPL_KMEM_H_

#include_next <sys/kmem.h>
#include <sys/systm.h>

/* NetBSD pool caches already participate in memory reclamation. */
#define	KMC_RECLAIMABLE	0

#undef kmem_strdup
#define	kmem_strdup(s)	kmem_strdupsize((s), NULL, KM_SLEEP)

/* snprintf returns the required length; scnprintf returns bytes stored. */
static inline int __printflike(3, 4)
kmem_scnprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list ap;
	int n;

	if (size == 0)
		return (0);
	va_start(ap, fmt);
	n = vsnprintf(buf, size, fmt, ap);
	va_end(ap);
	return (n < 0 ? 0 : MIN((size_t)n, size - 1));
}

#endif

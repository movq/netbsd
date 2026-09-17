/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_KMEM_H_
#define	_NETBSD_SPL_KMEM_H_

#include <sys/sys/kmem.h>
#include <sys/pool.h>
#include <sys/vmem.h>
#include <sys/systm.h>

#define	KM_PUSHPAGE	KM_SLEEP
#define	KM_NORMALPRI	0
#define	KM_NODEBUG	0
#define	KMC_NOTOUCH	0x00010000
#define	KMC_NODEBUG	0x00020000
/* NetBSD pool caches already participate in memory reclamation. */
#define	KMC_RECLAIMABLE	0

#define	POINTER_IS_VALID(p)	(!((uintptr_t)(p) & 0x3))
#define	POINTER_INVALIDATE(pp)	(*(pp) = (void *)((uintptr_t)(*(pp)) | 0x1))
#define	heap_arena	kmem_arena

/* Retain the shared Solaris wrappers' zero-size allocation semantics. */
#define	kmem_alloc	solaris_kmem_alloc
#define	kmem_zalloc	solaris_kmem_zalloc
#define	kmem_free	solaris_kmem_free
#define	kmem_size()	((uint64_t)physmem * PAGE_SIZE)
void *solaris_kmem_alloc(size_t, int);
void *solaris_kmem_zalloc(size_t, int);
void solaris_kmem_free(void *, size_t);

typedef struct openzfs_kmem_cache kmem_cache_t;
#define	kmem_cache_create	openzfs_kmem_cache_create
#define	kmem_cache_destroy	openzfs_kmem_cache_destroy
#define	kmem_cache_alloc		openzfs_kmem_cache_alloc
#define	kmem_cache_free		openzfs_kmem_cache_free
#define	kmem_cache_reap_now	openzfs_kmem_cache_reap_now
#define	kmem_cache_reap_active	openzfs_kmem_cache_reap_active
#define	spl_kmem_cache_inuse	openzfs_kmem_cache_inuse
#define	spl_kmem_cache_entry_size	openzfs_kmem_cache_entry_size

kmem_cache_t *kmem_cache_create(const char *, size_t, size_t,
    int (*)(void *, void *, int), void (*)(void *, void *),
    void (*)(void *), void *, vmem_t *, int);
void kmem_cache_destroy(kmem_cache_t *);
void *kmem_cache_alloc(kmem_cache_t *, int);
void kmem_cache_free(kmem_cache_t *, void *);
void kmem_cache_reap_now(kmem_cache_t *);
boolean_t kmem_cache_reap_active(void);
uint64_t spl_kmem_cache_inuse(kmem_cache_t *);
uint64_t spl_kmem_cache_entry_size(kmem_cache_t *);

/* Native pools neither move objects nor implement Solaris kmem_flags. */
#define	kmem_cache_set_move(cache, func)	((void)0)
#define	kmem_debugging()	0

#undef kmem_strdup
#define	kmem_strdup(s)	kmem_strdupsize((s), NULL, KM_SLEEP)

#define	kmem_vasprintf	openzfs_kmem_vasprintf
char *kmem_vasprintf(const char *, va_list) __printflike(1, 0);

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

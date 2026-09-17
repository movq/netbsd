/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/debug.h>
#include <sys/kmem.h>

/* Adapted from osnet/sys/kern/kmem.c; the backing allocator is unchanged. */
struct openzfs_kmem_cache {
	pool_cache_t	km_pool;
	char		km_name[32];
	size_t		km_size;
	void		*km_private;
	int		(*km_constructor)(void *, void *, int);
	void		(*km_destructor)(void *, void *);
	void		(*km_reclaim)(void *);
};

static unsigned cache_reaps;

static int
cache_constructor(void *private, void *object, int flags)
{
	kmem_cache_t *km = private;

	return (km->km_constructor != NULL ?
	    km->km_constructor(object, km->km_private, flags) : 0);
}

static void
cache_destructor(void *private, void *object)
{
	kmem_cache_t *km = private;

	if (km->km_destructor != NULL)
		km->km_destructor(object, km->km_private);
}

static void
cache_reclaim(void *private, int flags)
{
	kmem_cache_t *km = private;

	km->km_reclaim(km->km_private);
}

kmem_cache_t *
kmem_cache_create(const char *name, size_t size, size_t align,
    int (*constructor)(void *, void *, int), void (*destructor)(void *, void *),
    void (*reclaim)(void *), void *private, vmem_t *arena, int flags)
{
	kmem_cache_t *km;

	VERIFY0(flags & ~(KMC_NOTOUCH | KMC_NODEBUG));
	VERIFY3P(arena, ==, NULL);
	VERIFY3U(align, <=, UINT_MAX);
	km = kmem_zalloc(sizeof (*km), KM_SLEEP);
	strlcpy(km->km_name, name, sizeof (km->km_name));
	km->km_size = size;
	km->km_private = private;
	km->km_constructor = constructor;
	km->km_destructor = destructor;
	km->km_reclaim = reclaim;
	km->km_pool = pool_cache_init(size, align, 0, 0, km->km_name, NULL,
	    IPL_NONE, cache_constructor, cache_destructor, km);
	if (km->km_pool == NULL) {
		kmem_free(km, sizeof (*km));
		return (NULL);
	}
	if (reclaim != NULL)
		pool_cache_set_drain_hook(km->km_pool, cache_reclaim, km);
	return (km);
}

void
kmem_cache_destroy(kmem_cache_t *km)
{
	pool_cache_destroy(km->km_pool);
	kmem_free(km, sizeof (*km));
}

void *
kmem_cache_alloc(kmem_cache_t *km, int flags)
{
	VERIFY0(flags & ~(KM_SLEEP | KM_NOSLEEP | KM_PUSHPAGE));
	return (pool_cache_get(km->km_pool, flags));
}

void
kmem_cache_free(kmem_cache_t *km, void *object)
{
	pool_cache_put(km->km_pool, object);
}

void
kmem_cache_reap_now(kmem_cache_t *km)
{
	atomic_inc_uint(&cache_reaps);
	(void) pool_cache_reclaim(km->km_pool);
	atomic_dec_uint(&cache_reaps);
}

boolean_t
kmem_cache_reap_active(void)
{
	return (atomic_load_relaxed(&cache_reaps) != 0);
}

uint64_t
spl_kmem_cache_inuse(kmem_cache_t *km)
{
	struct pool *pool = &km->km_pool->pc_pool;
	uint64_t count;

	/* Include objects retained in CPU caches, as well as active objects. */
	mutex_spin_enter(&pool->pr_lock);
	count = pool->pr_nout;
	mutex_spin_exit(&pool->pr_lock);
	return (count);
}

uint64_t
spl_kmem_cache_entry_size(kmem_cache_t *km)
{
	return (km->km_size);
}

/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef CACHE_TEST_SHIM_H
#define	CACHE_TEST_SHIM_H

#include "taskq_shim.h"

#define	__printflike(a, b)	__attribute__((format(printf, a, b)))
#define	MIN(a, b)	((a) < (b) ? (a) : (b))
#define	atomic_inc_uint(p)	((void)__atomic_add_fetch(p, 1, __ATOMIC_RELAXED))
#define	atomic_dec_uint(p)	((void)__atomic_sub_fetch(p, 1, __ATOMIC_RELAXED))
#define	atomic_load_relaxed(p)	__atomic_load_n(p, __ATOMIC_RELAXED)
#define	mutex_spin_enter	mutex_enter
#define	mutex_spin_exit		mutex_exit
#define	IPL_NONE	0
typedef struct vmem vmem_t;

struct pool {
	kmutex_t pr_lock;
	unsigned pr_nout;
};
struct test_object {
	struct test_object *next;
	void *data;
};
typedef struct pool_cache {
	struct pool pc_pool;
	size_t size;
	unsigned align;
	unsigned active;
	void *private;
	int (*constructor)(void *, void *, int);
	void (*destructor)(void *, void *);
	void (*drain)(void *, int);
	void *drain_arg;
	struct test_object *objects;
} *pool_cache_t;
struct pool_allocator;
pool_cache_t pool_cache_init(size_t, unsigned, unsigned, unsigned,
    const char *, struct pool_allocator *, int,
    int (*)(void *, void *, int), void (*)(void *, void *), void *);
void pool_cache_destroy(pool_cache_t);
void *pool_cache_get(pool_cache_t, int);
void pool_cache_put(pool_cache_t, void *);
bool pool_cache_reclaim(pool_cache_t);
void pool_cache_set_drain_hook(pool_cache_t, void (*)(void *, int), void *);
size_t strlcpy(char *, const char *, size_t);

#endif

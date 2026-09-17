/* SPDX-License-Identifier: BSD-2-Clause */
#include "cache_shim.h"
#include <sys/kmem.h>

#undef kmem_alloc
#undef kmem_zalloc
#undef kmem_free

void *solaris_kmem_alloc(size_t n, int f) { return (kmem_alloc(n, f)); }
void *solaris_kmem_zalloc(size_t n, int f) { return (kmem_zalloc(n, f)); }
void solaris_kmem_free(void *p, size_t n) { kmem_free(p, n); }

size_t
strlcpy(char *dst, const char *src, size_t size)
{
	size_t len = strlen(src);
	if (size != 0) {
		size_t copy = MIN(len, size - 1);
		memcpy(dst, src, copy);
		dst[copy] = '\0';
	}
	return (len);
}

static bool fail_create, fail_get;

pool_cache_t
pool_cache_init(size_t size, unsigned align, unsigned offset, unsigned flags,
    const char *name, struct pool_allocator *pa, int ipl,
    int (*ctor)(void *, void *, int), void (*dtor)(void *, void *), void *arg)
{
	if (fail_create)
		return (NULL);
	pool_cache_t pc = kmem_zalloc(sizeof (*pc), KM_SLEEP);
	mutex_init(&pc->pc_pool.pr_lock, 0, 0, 0);
	pc->size = size;
	pc->align = MAX(align, sizeof (void *));
	pc->constructor = ctor;
	pc->destructor = dtor;
	pc->private = arg;
	return (pc);
}

void *
pool_cache_get(pool_cache_t pc, int flags)
{
	void *data;
	if (fail_get)
		return (NULL);
	if (pc->objects != NULL) {
		struct test_object *o = pc->objects;
		data = o->data;
		pc->objects = o->next;
		kmem_free(o, sizeof (*o));
	} else {
		VERIFY0(posix_memalign(&data, pc->align, pc->size));
		if (pc->constructor(pc->private, data, flags) != 0) {
			free(data);
			return (NULL);
		}
		pc->pc_pool.pr_nout++;
	}
	pc->active++;
	return (data);
}

void
pool_cache_put(pool_cache_t pc, void *data)
{
	assert(pc->active != 0);
	pc->active--;
	struct test_object *o = kmem_alloc(sizeof (*o), KM_SLEEP);
	o->data = data;
	o->next = pc->objects;
	pc->objects = o;
}

static void
drain_objects(pool_cache_t pc)
{
	while (pc->objects != NULL) {
		struct test_object *o = pc->objects;
		pc->objects = o->next;
		pc->destructor(pc->private, o->data);
		free(o->data);
		pc->pc_pool.pr_nout--;
		kmem_free(o, sizeof (*o));
	}
}

bool
pool_cache_reclaim(pool_cache_t pc)
{
	if (pc->drain != NULL)
		pc->drain(pc->drain_arg, KM_NOSLEEP);
	drain_objects(pc);
	return (true);
}

void
pool_cache_destroy(pool_cache_t pc)
{
	assert(pc->active == 0);
	drain_objects(pc);
	assert(pc->pc_pool.pr_nout == 0);
	mutex_destroy(&pc->pc_pool.pr_lock);
	kmem_free(pc, sizeof (*pc));
}

void
pool_cache_set_drain_hook(pool_cache_t pc, void (*fn)(void *, int), void *arg)
{
	pc->drain = fn;
	pc->drain_arg = arg;
}

struct counters {
	int constructed, destroyed, reaped, flags;
	bool fail;
};

static int
construct(void *obj, void *arg, int flags)
{
	struct counters *c = arg;
	c->flags = flags;
	if (c->fail)
		return (ENOMEM);
	*(uint64_t *)obj = 0x12345678;
	c->constructed++;
	return (0);
}

static void
destruct(void *obj, void *arg)
{
	struct counters *c = arg;
	assert(*(uint64_t *)obj == 0x12345678);
	c->destroyed++;
}

static void
reap(void *arg)
{
	struct counters *c = arg;
	assert(kmem_cache_reap_active());
	c->reaped++;
}

int
main(void)
{
	struct counters c = {0};
	fail_create = true;
	assert(kmem_cache_create("failed", 64, 64, NULL, NULL, NULL,
	    NULL, NULL, 0) == NULL);
	assert(test_allocations == 0);
	fail_create = false;
	kmem_cache_t *km = kmem_cache_create("test", 64, 64, construct,
	    destruct, reap, &c, NULL, KMC_NODEBUG);
	assert(km != NULL);
	assert(spl_kmem_cache_entry_size(km) == 64);
	assert(spl_kmem_cache_inuse(km) == 0);
	fail_get = true;
	assert(kmem_cache_alloc(km, KM_NOSLEEP) == NULL);
	fail_get = false;
	c.fail = true;
	assert(kmem_cache_alloc(km, KM_NOSLEEP) == NULL);
	assert(c.flags == KM_NOSLEEP);
	assert(spl_kmem_cache_inuse(km) == 0);
	c.fail = false;
	void *a = kmem_cache_alloc(km, KM_SLEEP);
	void *b = kmem_cache_alloc(km, KM_NOSLEEP);
	assert(((uintptr_t)a & 63) == 0 && ((uintptr_t)b & 63) == 0);
	assert(c.constructed == 2);
	assert(spl_kmem_cache_inuse(km) == 2);
	kmem_cache_free(km, a);
	/* Retained objects remain accounted until the pool releases them. */
	assert(spl_kmem_cache_inuse(km) == 2);
	assert(kmem_cache_alloc(km, KM_SLEEP) == a);
	assert(c.constructed == 2);
	kmem_cache_free(km, a);
	assert(!kmem_cache_reap_active());
	kmem_cache_reap_now(km);
	assert(!kmem_cache_reap_active());
	assert(c.reaped == 1 && c.destroyed == 1);
	assert(spl_kmem_cache_inuse(km) == 1);
	kmem_cache_free(km, b);
	kmem_cache_destroy(km);
	assert(c.destroyed == 2);
	assert(test_allocations == 0);
	puts("kmem cache: callbacks, failures, alignment, retained-object "
	    "accounting and reclaim tests passed");
	return (0);
}

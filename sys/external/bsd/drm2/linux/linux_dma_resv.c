/*	$NetBSD$	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Copyright (C) 2012-2014 Canonical Ltd (Maarten Lankhorst)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/poll.h>
#include <sys/select.h>

#include <linux/bug.h>
#include <linux/dma-fence-array.h>
#include <linux/dma-resv.h>
#include <linux/log2.h>
#include <linux/slab.h>
#include <linux/ww_mutex.h>

DEFINE_WW_CLASS(reservation_ww_class);

#define	DMA_RESV_LIST_MASK	0x3

struct dma_resv_list {
	struct rcu_head		rcu;
	uint32_t		num_fences;
	uint32_t		max_fences;
	struct dma_fence __rcu	*table[];
};

static void
dma_resv_list_entry(struct dma_resv_list *list, unsigned int index,
    struct dma_fence **fence, enum dma_resv_usage *usage)
{
	uintptr_t entry;

	entry = (uintptr_t)rcu_dereference(list->table[index]);
	*fence = (struct dma_fence *)(entry & ~DMA_RESV_LIST_MASK);
	if (usage != NULL)
		*usage = entry & DMA_RESV_LIST_MASK;
}

static void
dma_resv_list_set(struct dma_resv_list *list, unsigned int index,
    struct dma_fence *fence, enum dma_resv_usage usage)
{
	uintptr_t entry = (uintptr_t)fence | usage;

	RCU_INIT_POINTER(list->table[index], (struct dma_fence *)entry);
}

static struct dma_resv_list *
dma_resv_list_alloc(unsigned int max_fences)
{
	struct dma_resv_list *list;
	size_t size;

	if (max_fences > (SIZE_MAX - offsetof(struct dma_resv_list, table)) /
	    sizeof(list->table[0]))
		return NULL;
	size = offsetof(struct dma_resv_list, table[max_fences]);
	list = kmalloc(size, GFP_KERNEL);
	if (list == NULL)
		return NULL;
	list->num_fences = 0;
	list->max_fences = max_fences;
	return list;
}

static void
dma_resv_list_free(struct dma_resv_list *list)
{
	struct dma_fence *fence;
	unsigned int i;

	if (list == NULL)
		return;
	for (i = 0; i < list->num_fences; i++) {
		dma_resv_list_entry(list, i, &fence, NULL);
		dma_fence_put(fence);
	}
	kfree_rcu(list, rcu);
}

static struct dma_resv_list *
dma_resv_fences_list(struct dma_resv *obj)
{
	return rcu_dereference(obj->fences);
}

void
dma_resv_init(struct dma_resv *obj)
{
	ww_mutex_init(&obj->lock, &reservation_ww_class);
	RCU_INIT_POINTER(obj->fences, NULL);
}

void
dma_resv_fini(struct dma_resv *obj)
{
	dma_resv_list_free(rcu_dereference_protected(obj->fences, true));
	ww_mutex_destroy(&obj->lock);
}

int
dma_resv_lock(struct dma_resv *obj, struct ww_acquire_ctx *ctx)
{
	return ww_mutex_lock(&obj->lock, ctx);
}

void
dma_resv_lock_slow(struct dma_resv *obj, struct ww_acquire_ctx *ctx)
{
	ww_mutex_lock_slow(&obj->lock, ctx);
}

int
dma_resv_lock_interruptible(struct dma_resv *obj,
    struct ww_acquire_ctx *ctx)
{
	return ww_mutex_lock_interruptible(&obj->lock, ctx);
}

int
dma_resv_lock_slow_interruptible(struct dma_resv *obj,
    struct ww_acquire_ctx *ctx)
{
	return ww_mutex_lock_slow_interruptible(&obj->lock, ctx);
}

bool
dma_resv_trylock(struct dma_resv *obj)
{
	return ww_mutex_trylock(&obj->lock, NULL);
}

struct ww_acquire_ctx *
dma_resv_locking_ctx(struct dma_resv *obj)
{
	return ww_mutex_locking_ctx(&obj->lock);
}

void
dma_resv_unlock(struct dma_resv *obj)
{
	ww_mutex_unlock(&obj->lock);
}

bool
dma_resv_is_locked(struct dma_resv *obj)
{
	return ww_mutex_is_locked(&obj->lock);
}

bool
dma_resv_held(struct dma_resv *obj)
{
	return ww_mutex_is_locked(&obj->lock);
}

void
dma_resv_assert_held(struct dma_resv *obj)
{
	KASSERT(dma_resv_held(obj));
}

int
dma_resv_reserve_fences(struct dma_resv *obj, unsigned int num_fences)
{
	struct dma_resv_list *old, *new;
	struct dma_fence *fence;
	enum dma_resv_usage usage;
	unsigned int i, j, k, max_fences;

	dma_resv_assert_held(obj);
	if (WARN_ON(num_fences == 0))
		return -EINVAL;

	old = dma_resv_fences_list(obj);
	if (old != NULL && old->max_fences != 0) {
		if (num_fences <= old->max_fences - old->num_fences)
			return 0;
		if (old->max_fences > UINT_MAX / 2)
			return -ENOMEM;
		max_fences = MAX(old->num_fences + num_fences,
		    old->max_fences * 2);
	} else {
		max_fences = MAX(4UL, roundup_pow_of_two(num_fences));
	}

	new = dma_resv_list_alloc(max_fences);
	if (new == NULL)
		return -ENOMEM;

	for (i = 0, j = 0, k = max_fences;
	    i < (old != NULL ? old->num_fences : 0); i++) {
		dma_resv_list_entry(old, i, &fence, &usage);
		if (dma_fence_is_signaled(fence))
			RCU_INIT_POINTER(new->table[--k], fence);
		else
			dma_resv_list_set(new, j++, fence, usage);
	}
	new->num_fences = j;
	rcu_assign_pointer(obj->fences, new);

	if (old == NULL)
		return 0;
	for (i = k; i < max_fences; i++) {
		fence = rcu_dereference_protected(new->table[i], true);
		dma_fence_put(fence);
	}
	kfree_rcu(old, rcu);
	return 0;
}

void
dma_resv_add_fence(struct dma_resv *obj, struct dma_fence *fence,
    enum dma_resv_usage usage)
{
	struct dma_resv_list *list;
	struct dma_fence *old;
	enum dma_resv_usage old_usage;
	unsigned int i;

	dma_resv_assert_held(obj);
	list = dma_resv_fences_list(obj);
	KASSERT(list != NULL);
	dma_fence_get(fence);

	for (i = 0; i < list->num_fences; i++) {
		dma_resv_list_entry(list, i, &old, &old_usage);
		if ((old->context == fence->context && old_usage >= usage &&
		    (old->seqno == fence->seqno ||
		    dma_fence_is_later(fence, old))) ||
		    dma_fence_is_signaled(old)) {
			dma_resv_list_set(list, i, fence, usage);
			dma_fence_put(old);
			return;
		}
	}

	KASSERT(list->num_fences < list->max_fences);
	dma_resv_list_set(list, i, fence, usage);
	membar_producer();
	list->num_fences++;
}

void
dma_resv_replace_fences(struct dma_resv *obj, uint64_t context,
    struct dma_fence *replacement, enum dma_resv_usage usage)
{
	struct dma_resv_list *list;
	struct dma_fence *old;
	unsigned int i;

	dma_resv_assert_held(obj);
	list = dma_resv_fences_list(obj);
	for (i = 0; list != NULL && i < list->num_fences; i++) {
		dma_resv_list_entry(list, i, &old, NULL);
		if (old->context != context)
			continue;
		dma_resv_list_set(list, i, dma_fence_get(replacement), usage);
		dma_fence_put(old);
	}
}

static void
dma_resv_iter_restart_unlocked(struct dma_resv_iter *cursor)
{
	cursor->index = 0;
	cursor->num_fences = 0;
	cursor->fences = dma_resv_fences_list(cursor->obj);
	if (cursor->fences != NULL)
		cursor->num_fences = cursor->fences->num_fences;
	cursor->is_restarted = true;
}

static void
dma_resv_iter_walk_unlocked(struct dma_resv_iter *cursor)
{
	if (cursor->fences == NULL)
		return;

	for (;;) {
		dma_fence_put(cursor->fence);
		if (cursor->index >= cursor->num_fences) {
			cursor->fence = NULL;
			return;
		}

		dma_resv_list_entry(cursor->fences, cursor->index++,
		    &cursor->fence, &cursor->fence_usage);
		cursor->fence = dma_fence_get_rcu(cursor->fence);
		if (cursor->fence == NULL) {
			dma_resv_iter_restart_unlocked(cursor);
			continue;
		}
		if (!dma_fence_is_signaled(cursor->fence) &&
		    cursor->fence_usage <= cursor->usage)
			return;
	}
}

struct dma_fence *
dma_resv_iter_first_unlocked(struct dma_resv_iter *cursor)
{
	rcu_read_lock();
	do {
		dma_resv_iter_restart_unlocked(cursor);
		dma_resv_iter_walk_unlocked(cursor);
	} while (dma_resv_fences_list(cursor->obj) != cursor->fences);
	rcu_read_unlock();
	return cursor->fence;
}

struct dma_fence *
dma_resv_iter_next_unlocked(struct dma_resv_iter *cursor)
{
	bool restart;

	rcu_read_lock();
	cursor->is_restarted = false;
	restart = dma_resv_fences_list(cursor->obj) != cursor->fences;
	do {
		if (restart)
			dma_resv_iter_restart_unlocked(cursor);
		dma_resv_iter_walk_unlocked(cursor);
		restart = true;
	} while (dma_resv_fences_list(cursor->obj) != cursor->fences);
	rcu_read_unlock();
	return cursor->fence;
}

struct dma_fence *
dma_resv_iter_first(struct dma_resv_iter *cursor)
{
	struct dma_fence *fence;

	dma_resv_assert_held(cursor->obj);
	cursor->index = 0;
	cursor->fences = dma_resv_fences_list(cursor->obj);
	fence = dma_resv_iter_next(cursor);
	cursor->is_restarted = true;
	return fence;
}

struct dma_fence *
dma_resv_iter_next(struct dma_resv_iter *cursor)
{
	struct dma_fence *fence;

	dma_resv_assert_held(cursor->obj);
	cursor->is_restarted = false;
	do {
		if (cursor->fences == NULL ||
		    cursor->index >= cursor->fences->num_fences)
			return NULL;
		dma_resv_list_entry(cursor->fences, cursor->index++, &fence,
		    &cursor->fence_usage);
	} while (cursor->fence_usage > cursor->usage);
	return fence;
}

int
dma_resv_copy_fences(struct dma_resv *dst, struct dma_resv *src)
{
	struct dma_resv_iter cursor;
	struct dma_resv_list *list = NULL, *old;
	struct dma_fence *fence;

	dma_resv_assert_held(dst);
	dma_resv_iter_begin(&cursor, src, DMA_RESV_USAGE_BOOKKEEP);
	dma_resv_for_each_fence_unlocked(&cursor, fence) {
		if (dma_resv_iter_is_restarted(&cursor)) {
			dma_resv_list_free(list);
			list = dma_resv_list_alloc(cursor.num_fences);
			if (list == NULL) {
				dma_resv_iter_end(&cursor);
				return -ENOMEM;
			}
		}
		dma_resv_list_set(list, list->num_fences++,
		    dma_fence_get(fence), dma_resv_iter_usage(&cursor));
	}
	dma_resv_iter_end(&cursor);

	old = rcu_replace_pointer(dst->fences, list, dma_resv_held(dst));
	dma_resv_list_free(old);
	return 0;
}

int
dma_resv_get_fences(struct dma_resv *obj, enum dma_resv_usage usage,
    unsigned int *num_fences, struct dma_fence ***fences)
{
	struct dma_resv_iter cursor;
	struct dma_fence **new_fences;
	struct dma_fence *fence;
	unsigned int count;

	*num_fences = 0;
	*fences = NULL;
	dma_resv_iter_begin(&cursor, obj, usage);
	dma_resv_for_each_fence_unlocked(&cursor, fence) {
		if (dma_resv_iter_is_restarted(&cursor)) {
			while (*num_fences != 0)
				dma_fence_put((*fences)[--*num_fences]);
			count = cursor.num_fences + 1;
			new_fences = krealloc(*fences,
			    count * sizeof(**fences), GFP_KERNEL);
			if (new_fences == NULL) {
				kfree(*fences);
				*fences = NULL;
				dma_resv_iter_end(&cursor);
				return -ENOMEM;
			}
			*fences = new_fences;
		}
		(*fences)[(*num_fences)++] = dma_fence_get(fence);
	}
	dma_resv_iter_end(&cursor);
	return 0;
}

int
dma_resv_get_singleton(struct dma_resv *obj, enum dma_resv_usage usage,
    struct dma_fence **fence)
{
	struct dma_fence_array *array;
	struct dma_fence **fences;
	unsigned int count;
	int error;

	error = dma_resv_get_fences(obj, usage, &count, &fences);
	if (error)
		return error;
	if (count == 0) {
		*fence = NULL;
		return 0;
	}
	if (count == 1) {
		*fence = fences[0];
		kfree(fences);
		return 0;
	}

	array = dma_fence_array_create(count, fences,
	    dma_fence_context_alloc(1), 1, false);
	if (array == NULL) {
		while (count != 0)
			dma_fence_put(fences[--count]);
		kfree(fences);
		return -ENOMEM;
	}
	*fence = &array->base;
	return 0;
}

long
dma_resv_wait_timeout(struct dma_resv *obj, enum dma_resv_usage usage,
    bool intr, unsigned long timeout)
{
	struct dma_resv_iter cursor;
	struct dma_fence *fence;
	long ret = timeout != 0 ? timeout : 1;

	dma_resv_iter_begin(&cursor, obj, usage);
	dma_resv_for_each_fence_unlocked(&cursor, fence) {
		ret = dma_fence_wait_timeout(fence, intr, timeout);
		if (ret <= 0)
			break;
		if (timeout != 0)
			timeout = ret;
	}
	dma_resv_iter_end(&cursor);
	return ret;
}

void
dma_resv_set_deadline(struct dma_resv *obj, enum dma_resv_usage usage,
    ktime_t deadline)
{
	struct dma_resv_iter cursor;
	struct dma_fence *fence;

	dma_resv_iter_begin(&cursor, obj, usage);
	dma_resv_for_each_fence_unlocked(&cursor, fence)
		dma_fence_set_deadline(fence, deadline);
	dma_resv_iter_end(&cursor);
}

bool
dma_resv_test_signaled(struct dma_resv *obj, enum dma_resv_usage usage)
{
	struct dma_resv_iter cursor;
	struct dma_fence *fence;

	dma_resv_iter_begin(&cursor, obj, usage);
	dma_resv_for_each_fence_unlocked(&cursor, fence) {
		dma_resv_iter_end(&cursor);
		return false;
	}
	dma_resv_iter_end(&cursor);
	return true;
}

void
dma_resv_describe(struct dma_resv *obj, struct seq_file *seq)
{
	/* NetBSD does not yet provide dma_fence_describe. */
}

void
dma_resv_poll_init(struct dma_resv_poll *rpoll)
{
	mutex_init(&rpoll->rp_lock, MUTEX_DEFAULT, IPL_VM);
	selinit(&rpoll->rp_selq);
	rpoll->rp_claimed = false;
}

void
dma_resv_poll_fini(struct dma_resv_poll *rpoll)
{
	KASSERT(!rpoll->rp_claimed);
	seldestroy(&rpoll->rp_selq);
	mutex_destroy(&rpoll->rp_lock);
}

static void
dma_resv_poll_cb(struct dma_fence *fence, struct dma_fence_cb *fcb)
{
	struct dma_resv_poll *rpoll = container_of(fcb,
	    struct dma_resv_poll, rp_fcb);

	mutex_enter(&rpoll->rp_lock);
	selnotify(&rpoll->rp_selq, 0, NOTE_SUBMIT);
	rpoll->rp_claimed = false;
	mutex_exit(&rpoll->rp_lock);
}

int
dma_resv_do_poll(const struct dma_resv *obj, int events,
    struct dma_resv_poll *rpoll)
{
	struct dma_resv_iter cursor;
	struct dma_fence *fence;
	enum dma_resv_usage usage;
	bool claimed;
	int revents;

top:
	revents = 0;
	if ((events & POLLIN) &&
	    dma_resv_test_signaled(__UNCONST(obj), DMA_RESV_USAGE_WRITE))
		revents |= POLLIN;
	if ((events & POLLOUT) &&
	    dma_resv_test_signaled(__UNCONST(obj), DMA_RESV_USAGE_READ))
		revents |= POLLOUT;
	if (revents == (events & (POLLIN | POLLOUT)))
		return revents;

	mutex_enter(&rpoll->rp_lock);
	selrecord(curlwp, &rpoll->rp_selq);
	claimed = !rpoll->rp_claimed;
	if (claimed)
		rpoll->rp_claimed = true;
	mutex_exit(&rpoll->rp_lock);
	if (!claimed)
		return revents;

	usage = ((events & POLLOUT) && !(revents & POLLOUT)) ?
	    DMA_RESV_USAGE_READ : DMA_RESV_USAGE_WRITE;
	dma_resv_iter_begin(&cursor, __UNCONST(obj), usage);
	dma_resv_for_each_fence_unlocked(&cursor, fence) {
		if (dma_fence_add_callback(fence, &rpoll->rp_fcb,
		    dma_resv_poll_cb) == 0) {
			dma_resv_iter_end(&cursor);
			return revents;
		}
	}
	dma_resv_iter_end(&cursor);

	/* Every candidate raced to completion after selrecord. */
	dma_resv_poll_cb(NULL, &rpoll->rp_fcb);
	goto top;
}

int
dma_resv_kqfilter(const struct dma_resv *obj, struct knote *kn,
    struct dma_resv_poll *rpoll)
{
	return EINVAL;
}

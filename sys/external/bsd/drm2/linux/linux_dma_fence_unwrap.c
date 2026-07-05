/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2022 Advanced Micro Devices, Inc.
 * Authors:
 *	Christian König <christian.koenig@amd.com>
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <linux/dma-fence.h>
#include <linux/dma-fence-array.h>
#include <linux/dma-fence-chain.h>
#include <linux/dma-fence-unwrap.h>
#include <linux/slab.h>
#include <linux/sort.h>

static struct dma_fence *
dma_fence_unwrap_array(struct dma_fence_unwrap *cursor)
{

	cursor->array = dma_fence_chain_contained(cursor->chain);
	cursor->index = 0;
	return dma_fence_array_first(cursor->array);
}

struct dma_fence *
dma_fence_unwrap_first(struct dma_fence *head,
    struct dma_fence_unwrap *cursor)
{

	cursor->chain = dma_fence_get(head);
	return dma_fence_unwrap_array(cursor);
}

struct dma_fence *
dma_fence_unwrap_next(struct dma_fence_unwrap *cursor)
{
	struct dma_fence *fence;

	fence = dma_fence_array_next(cursor->array, ++cursor->index);
	if (fence != NULL)
		return fence;

	cursor->chain = dma_fence_chain_walk(cursor->chain);
	return dma_fence_unwrap_array(cursor);
}

static int
fence_cmp(const void *va, const void *vb)
{
	struct dma_fence *a = *(struct dma_fence * const *)va;
	struct dma_fence *b = *(struct dma_fence * const *)vb;

	if (a->context < b->context)
		return -1;
	if (a->context > b->context)
		return 1;
	if (dma_fence_is_later(b, a))
		return 1;
	if (dma_fence_is_later(a, b))
		return -1;
	return 0;
}

int
dma_fence_dedup_array(struct dma_fence **fences, int num_fences)
{
	int i, j;

	sort(fences, num_fences, sizeof(*fences), fence_cmp, NULL);

	j = 0;
	for (i = 1; i < num_fences; i++) {
		if (fences[i]->context == fences[j]->context)
			dma_fence_put(fences[i]);
		else
			fences[++j] = fences[i];
	}

	return ++j;
}

struct dma_fence *
__dma_fence_unwrap_merge(unsigned num_fences, struct dma_fence **fences,
    struct dma_fence_unwrap *iter)
{
	struct dma_fence *fence, *unsignaled = NULL, **array;
	struct dma_fence_array *result;
	ktime_t timestamp;
	unsigned i;
	int count;

	count = 0;
	timestamp = ns_to_ktime(0);
	for (i = 0; i < num_fences; i++) {
		dma_fence_unwrap_for_each(fence, &iter[i], fences[i]) {
			if (!dma_fence_is_signaled(fence)) {
				dma_fence_put(unsignaled);
				unsignaled = dma_fence_get(fence);
				count++;
			} else {
				ktime_t t = dma_fence_timestamp(fence);

				if (ktime_after(t, timestamp))
					timestamp = t;
			}
		}
	}

	if (count == 0)
		return dma_fence_allocate_private_stub(timestamp);
	if (count == 1)
		return unsignaled;

	dma_fence_put(unsignaled);
	array = kmalloc_array(count, sizeof(*array), GFP_KERNEL);
	if (array == NULL)
		return NULL;

	count = 0;
	for (i = 0; i < num_fences; i++) {
		dma_fence_unwrap_for_each(fence, &iter[i], fences[i]) {
			if (!dma_fence_is_signaled(fence)) {
				array[count++] = dma_fence_get(fence);
			} else {
				ktime_t t = dma_fence_timestamp(fence);

				if (ktime_after(t, timestamp))
					timestamp = t;
			}
		}
	}

	if (count == 0 || count == 1)
		goto return_fastpath;

	count = dma_fence_dedup_array(array, count);
	if (count > 1) {
		result = dma_fence_array_create(count, array,
		    dma_fence_context_alloc(1), 1, false);
		if (result == NULL) {
			for (i = 0; i < (unsigned)count; i++)
				dma_fence_put(array[i]);
			fence = NULL;
			goto return_fence;
		}
		return &result->base;
	}

return_fastpath:
	if (count == 0)
		fence = dma_fence_allocate_private_stub(timestamp);
	else
		fence = array[0];

return_fence:
	kfree(array);
	return fence;
}

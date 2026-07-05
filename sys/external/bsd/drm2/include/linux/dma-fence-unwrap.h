/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2022 Advanced Micro Devices, Inc.
 * Authors:
 *	Christian König <christian.koenig@amd.com>
 */

#ifndef _LINUX_DMA_FENCE_UNWRAP_H
#define _LINUX_DMA_FENCE_UNWRAP_H

struct dma_fence;

struct dma_fence_unwrap {
	struct dma_fence *chain;
	struct dma_fence *array;
	unsigned int index;
};

#define	dma_fence_unwrap_first		linux_dma_fence_unwrap_first
#define	dma_fence_unwrap_next		linux_dma_fence_unwrap_next
#define	__dma_fence_unwrap_merge	linux___dma_fence_unwrap_merge
#define	dma_fence_dedup_array		linux_dma_fence_dedup_array

struct dma_fence *
	dma_fence_unwrap_first(struct dma_fence *, struct dma_fence_unwrap *);
struct dma_fence *
	dma_fence_unwrap_next(struct dma_fence_unwrap *);
struct dma_fence *
	__dma_fence_unwrap_merge(unsigned, struct dma_fence **,
	    struct dma_fence_unwrap *);
int	dma_fence_dedup_array(struct dma_fence **, int);

#define	dma_fence_unwrap_for_each(FENCE, CURSOR, HEAD)			\
	for ((FENCE) = dma_fence_unwrap_first((HEAD), (CURSOR));		\
	     (FENCE) != NULL;						\
	     (FENCE) = dma_fence_unwrap_next((CURSOR)))

#define	dma_fence_unwrap_merge(...)					\
	({								\
		struct dma_fence *__f[] = { __VA_ARGS__ };		\
		struct dma_fence_unwrap __c[ARRAY_SIZE(__f)];		\
									\
		__dma_fence_unwrap_merge(ARRAY_SIZE(__f), __f, __c);	\
	})

#endif

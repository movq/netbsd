/*	$NetBSD$	*/

/*
 * Header file for reservations for dma-buf and ttm.
 *
 * Copyright(C) 2011 Linaro Limited. All rights reserved.
 * Copyright (C) 2012-2013 Canonical Ltd
 * Copyright (C) 2012 Texas Instruments
 */

#ifndef	_LINUX_DMA_RESV_H_
#define	_LINUX_DMA_RESV_H_

#include <sys/event.h>
#include <sys/mutex.h>
#include <sys/select.h>

#include <linux/dma-fence.h>
#include <linux/ktime.h>
#include <linux/rcupdate.h>
#include <linux/ww_mutex.h>

enum dma_resv_usage {
	DMA_RESV_USAGE_KERNEL,
	DMA_RESV_USAGE_WRITE,
	DMA_RESV_USAGE_READ,
	DMA_RESV_USAGE_BOOKKEEP
};

static inline enum dma_resv_usage
dma_resv_usage_rw(bool write)
{
	/*
	 * A writer waits for readers and writers, while a reader only waits
	 * for writers.
	 */
	return write ? DMA_RESV_USAGE_READ : DMA_RESV_USAGE_WRITE;
}

struct dma_resv_list;
struct seq_file;

struct dma_resv {
	struct ww_mutex			lock;
	struct dma_resv_list __rcu	*fences;
};

struct dma_resv_iter {
	struct dma_resv		*obj;
	enum dma_resv_usage	usage;
	struct dma_fence	*fence;
	enum dma_resv_usage	fence_usage;
	unsigned int		index;
	struct dma_resv_list	*fences;
	unsigned int		num_fences;
	bool			is_restarted;
};

/* NetBSD dma-buf poll state. */
struct dma_resv_poll {
	kmutex_t		rp_lock;
	struct selinfo		rp_selq;
	struct dma_fence_cb	rp_fcb;
	bool			rp_claimed;
};

#define	dma_resv_add_fence		linux_dma_resv_add_fence
#define	dma_resv_assert_held		linux_dma_resv_assert_held
#define	dma_resv_copy_fences		linux_dma_resv_copy_fences
#define	dma_resv_describe		linux_dma_resv_describe
#define	dma_resv_do_poll		linux_dma_resv_do_poll
#define	dma_resv_fini			linux_dma_resv_fini
#define	dma_resv_get_fences		linux_dma_resv_get_fences
#define	dma_resv_get_singleton		linux_dma_resv_get_singleton
#define	dma_resv_held			linux_dma_resv_held
#define	dma_resv_init			linux_dma_resv_init
#define	dma_resv_is_locked		linux_dma_resv_is_locked
#define	dma_resv_iter_first		linux_dma_resv_iter_first
#define	dma_resv_iter_first_unlocked	linux_dma_resv_iter_first_unlocked
#define	dma_resv_iter_next		linux_dma_resv_iter_next
#define	dma_resv_iter_next_unlocked	linux_dma_resv_iter_next_unlocked
#define	dma_resv_kqfilter		linux_dma_resv_kqfilter
#define	dma_resv_lock			linux_dma_resv_lock
#define	dma_resv_lock_interruptible	linux_dma_resv_lock_interruptible
#define	dma_resv_lock_slow		linux_dma_resv_lock_slow
#define	dma_resv_lock_slow_interruptible linux_dma_resv_lock_slow_interruptible
#define	dma_resv_locking_ctx		linux_dma_resv_locking_ctx
#define	dma_resv_poll_fini		linux_dma_resv_poll_fini
#define	dma_resv_poll_init		linux_dma_resv_poll_init
#define	dma_resv_replace_fences		linux_dma_resv_replace_fences
#define	dma_resv_reserve_fences		linux_dma_resv_reserve_fences
#define	dma_resv_set_deadline		linux_dma_resv_set_deadline
#define	dma_resv_test_signaled		linux_dma_resv_test_signaled
#define	dma_resv_trylock		linux_dma_resv_trylock
#define	dma_resv_unlock			linux_dma_resv_unlock
#define	dma_resv_wait_timeout		linux_dma_resv_wait_timeout
#define	reservation_ww_class		linux_reservation_ww_class

extern struct ww_class reservation_ww_class;

void	dma_resv_init(struct dma_resv *);
void	dma_resv_fini(struct dma_resv *);
int	dma_resv_lock(struct dma_resv *, struct ww_acquire_ctx *);
void	dma_resv_lock_slow(struct dma_resv *, struct ww_acquire_ctx *);
int	dma_resv_lock_interruptible(struct dma_resv *,
	    struct ww_acquire_ctx *);
int	dma_resv_lock_slow_interruptible(struct dma_resv *,
	    struct ww_acquire_ctx *);
bool	dma_resv_trylock(struct dma_resv *) __must_check;
struct ww_acquire_ctx *dma_resv_locking_ctx(struct dma_resv *);
void	dma_resv_unlock(struct dma_resv *);
bool	dma_resv_is_locked(struct dma_resv *);
bool	dma_resv_held(struct dma_resv *);
void	dma_resv_assert_held(struct dma_resv *);

struct dma_fence *dma_resv_iter_first_unlocked(struct dma_resv_iter *);
struct dma_fence *dma_resv_iter_next_unlocked(struct dma_resv_iter *);
struct dma_fence *dma_resv_iter_first(struct dma_resv_iter *);
struct dma_fence *dma_resv_iter_next(struct dma_resv_iter *);

int	dma_resv_reserve_fences(struct dma_resv *, unsigned int);
void	dma_resv_add_fence(struct dma_resv *, struct dma_fence *,
	    enum dma_resv_usage);
void	dma_resv_replace_fences(struct dma_resv *, uint64_t,
	    struct dma_fence *, enum dma_resv_usage);
int	dma_resv_copy_fences(struct dma_resv *, struct dma_resv *);
int	dma_resv_get_fences(struct dma_resv *, enum dma_resv_usage,
	    unsigned int *, struct dma_fence ***);
int	dma_resv_get_singleton(struct dma_resv *, enum dma_resv_usage,
	    struct dma_fence **);
long	dma_resv_wait_timeout(struct dma_resv *, enum dma_resv_usage,
	    bool, unsigned long);
void	dma_resv_set_deadline(struct dma_resv *, enum dma_resv_usage,
	    ktime_t);
bool	dma_resv_test_signaled(struct dma_resv *, enum dma_resv_usage);
void	dma_resv_describe(struct dma_resv *, struct seq_file *);

void	dma_resv_poll_init(struct dma_resv_poll *);
void	dma_resv_poll_fini(struct dma_resv_poll *);
int	dma_resv_do_poll(const struct dma_resv *, int,
	    struct dma_resv_poll *);
int	dma_resv_kqfilter(const struct dma_resv *, struct knote *,
	    struct dma_resv_poll *);

static inline void
dma_resv_iter_begin(struct dma_resv_iter *cursor, struct dma_resv *obj,
    enum dma_resv_usage usage)
{
	cursor->obj = obj;
	cursor->usage = usage;
	cursor->fence = NULL;
}

static inline void
dma_resv_iter_end(struct dma_resv_iter *cursor)
{
	dma_fence_put(cursor->fence);
}

static inline enum dma_resv_usage
dma_resv_iter_usage(struct dma_resv_iter *cursor)
{
	return cursor->fence_usage;
}

static inline bool
dma_resv_iter_is_restarted(struct dma_resv_iter *cursor)
{
	return cursor->is_restarted;
}

#define	dma_resv_for_each_fence_unlocked(CURSOR, FENCE)		      \
	for ((FENCE) = dma_resv_iter_first_unlocked(CURSOR);		      \
	    (FENCE); (FENCE) = dma_resv_iter_next_unlocked(CURSOR))

#define	dma_resv_for_each_fence(CURSOR, OBJ, USAGE, FENCE)		      \
	for (dma_resv_iter_begin((CURSOR), (OBJ), (USAGE)),		      \
	    (FENCE) = dma_resv_iter_first(CURSOR); (FENCE);		      \
	    (FENCE) = dma_resv_iter_next(CURSOR))

#endif	/* _LINUX_DMA_RESV_H_ */

/*	$NetBSD: xarray.h,v 1.9 2021/12/19 12:05:25 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_LINUX_XARRAY_H_
#define	_LINUX_XARRAY_H_

#include <sys/mutex.h>
#include <sys/rbtree.h>

#include <linux/slab.h>

struct xarray;
struct xa_limit;

struct xa_limit {
	uint32_t	max;
	uint32_t	min;
};

#define XA_LIMIT(_min, _max) (struct xa_limit) { .min = _min, .max = _max }

struct xarray {
	kmutex_t		xa_lock;
	struct rb_tree		xa_tree;
	gfp_t			xa_gfp;
};

#define	xa_for_each(XA, INDEX, ENTRY)					      \
	for ((INDEX) = 0, (ENTRY) = xa_find((XA), &(INDEX), ULONG_MAX, -1);   \
		(ENTRY) != NULL;					      \
		(ENTRY) = xa_find_after((XA), &(INDEX), ULONG_MAX, -1))

#define	XA_ERROR(error)	((void *)(((uintptr_t)error << 2) | 2))

static inline int
xa_err(void *cookie)
{

	if (((uintptr_t)cookie & 3) != 2)
		return 0;

	return (uintptr_t)cookie >> 2;
}

static inline bool
xa_is_err(const void *entry)
{
	return xa_err(__UNCONST(entry)) != 0;
}

static inline void *
xa_mk_value(unsigned long value)
{

	return (void *)((value << 1) | 1);
}

static inline bool
xa_is_value(const void *entry)
{

	return (uintptr_t)entry & 1;
}

static inline unsigned long
xa_to_value(const void *entry)
{

	return (uintptr_t)entry >> 1;
}

#define	XA_FLAGS_ALLOC	0
#define	XA_FLAGS_ALLOC1	0
#define	XA_FLAGS_LOCK_IRQ	0

#define	DEFINE_XARRAY_FLAGS(name, flags) \
	struct xarray name = { .xa_gfp = (flags) }

#define	DEFINE_XARRAY_ALLOC(name) \
	DEFINE_XARRAY_FLAGS(name, XA_FLAGS_ALLOC)
#define	DEFINE_XARRAY_ALLOC1(name) \
	DEFINE_XARRAY_FLAGS(name, XA_FLAGS_ALLOC1)

/* Lock/unlock for external callers that need atomic multi-operation sequences */
#define	xa_lock(_xa)		mutex_enter(&(_xa)->xa_lock)
#define	xa_unlock(_xa)		mutex_exit(&(_xa)->xa_lock)
#define	xa_lock_irq(_xa)	mutex_enter(&(_xa)->xa_lock)
#define	xa_unlock_irq(_xa)	mutex_exit(&(_xa)->xa_lock)
#define	xa_lock_irqsave(_xa, _flags) \
	do { (_flags) = 0; mutex_enter(&(_xa)->xa_lock); } while (0)
#define	xa_unlock_irqrestore(_xa, _flags) \
	do { (void)(_flags); mutex_exit(&(_xa)->xa_lock); } while (0)

/* Lock-free implementation functions */
#define	xa_limit_32b	linux_xa_limit_32b

void	linux_xa_init_flags(struct xarray *, gfp_t);
void	linux_xa_destroy(struct xarray *);

void *	linux_xa_load(struct xarray *, unsigned long);
void *	linux_xa_store(struct xarray *, unsigned long, void *, gfp_t);
void *	linux_xa_erase(struct xarray *, unsigned long);

int	linux_xa_alloc(struct xarray *, uint32_t *, void *, struct xa_limit,
	    gfp_t);
int	linux_xa_alloc_cyclic(struct xarray *, uint32_t *, void *,
	    struct xa_limit, uint32_t *, gfp_t);
void *	linux_xa_find(struct xarray *, unsigned long *, unsigned long, unsigned);
void *	linux_xa_find_after(struct xarray *, unsigned long *, unsigned long,
	    unsigned);

extern const struct xa_limit xa_limit_32b;

static inline void
xa_init_flags(struct xarray *xa, gfp_t gfp)
{
	linux_xa_init_flags(xa, gfp);
}

static inline void
xa_init(struct xarray *xa)
{
	linux_xa_init_flags(xa, 0);
}

static inline void
xa_destroy(struct xarray *xa)
{
	linux_xa_destroy(xa);
}

static inline void *
xa_load(struct xarray *xa, unsigned long index)
{
	return linux_xa_load(xa, index);
}

static inline bool
xa_empty(const struct xarray *xa)
{
	return xa->xa_tree.rbt_root == NULL;
}

static inline void *
xa_store(struct xarray *xa, unsigned long index, void *entry, gfp_t gfp)
{
	void *r;
	mutex_enter(&xa->xa_lock);
	r = linux_xa_store(xa, index, entry, gfp);
	mutex_exit(&xa->xa_lock);
	return r;
}

static inline void *
__xa_store(struct xarray *xa, unsigned long index, void *entry, gfp_t gfp)
{

	return linux_xa_store(xa, index, entry, gfp);
}

static inline void *
xa_store_irq(struct xarray *xa, unsigned long index, void *entry, gfp_t gfp)
{
	void *r;

	xa_lock_irq(xa);
	r = __xa_store(xa, index, entry, gfp);
	xa_unlock_irq(xa);
	return r;
}

static inline void *
xa_erase(struct xarray *xa, unsigned long index)
{
	void *r;
	mutex_enter(&xa->xa_lock);
	r = linux_xa_erase(xa, index);
	mutex_exit(&xa->xa_lock);
	return r;
}

static inline void *
__xa_erase(struct xarray *xa, unsigned long index)
{

	return linux_xa_erase(xa, index);
}

static inline void *
xa_erase_irq(struct xarray *xa, unsigned long index)
{
	void *r;

	xa_lock_irq(xa);
	r = __xa_erase(xa, index);
	xa_unlock_irq(xa);
	return r;
}

static inline int
xa_alloc(struct xarray *xa, uint32_t *id, void *entry, struct xa_limit xr,
    gfp_t gfp)
{
	int r;
	mutex_enter(&xa->xa_lock);
	r = linux_xa_alloc(xa, id, entry, xr, gfp);
	mutex_exit(&xa->xa_lock);
	return r;
}

static inline int
xa_alloc_cyclic_irq(struct xarray *xa, uint32_t *id, void *entry,
    struct xa_limit xr, uint32_t *next, gfp_t gfp)
{
	int r;

	mutex_enter(&xa->xa_lock);
	r = linux_xa_alloc_cyclic(xa, id, entry, xr, next, gfp);
	mutex_exit(&xa->xa_lock);
	return r;
}

static inline void *
xa_find(struct xarray *xa, unsigned long *start, unsigned long max,
    unsigned tagmask)
{
	void *r;
	mutex_enter(&xa->xa_lock);
	r = linux_xa_find(xa, start, max, tagmask);
	mutex_exit(&xa->xa_lock);
	return r;
}

static inline void *
xa_find_after(struct xarray *xa, unsigned long *start, unsigned long max,
    unsigned tagmask)
{
	void *r;
	mutex_enter(&xa->xa_lock);
	r = linux_xa_find_after(xa, start, max, tagmask);
	mutex_exit(&xa->xa_lock);
	return r;
}

#endif	/* _LINUX_XARRAY_H_ */

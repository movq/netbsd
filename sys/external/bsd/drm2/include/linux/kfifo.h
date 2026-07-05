/*	$NetBSD: kfifo.h,v 1.6 2021/12/19 12:33:02 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef	_LINUX_KFIFO_H_
#define	_LINUX_KFIFO_H_

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/mutex.h>

#include <linux/gfp.h>
#include <linux/slab.h>

struct kfifo_meta {
	kmutex_t	kfm_lock;
	size_t		kfm_head;
	size_t		kfm_tail;
	size_t		kfm_nbytes;
};

#define	_KFIFO_PTR_TYPE(TAG, TYPE)					      \
	struct TAG {							      \
		struct kfifo_meta	kf_meta;			      \
		TYPE			*kf_buf;			      \
	}

#define	_KFIFO_TYPE(TAG, TYPE, N)					      \
	struct TAG {							      \
		struct kfifo_meta	kf_meta;			      \
		TYPE			kf_buf[N];			      \
	}

#define	DECLARE_KFIFO_PTR(FIFO, TYPE)	_KFIFO_PTR_TYPE(, TYPE) FIFO
#define	DECLARE_KFIFO(FIFO, TYPE, N)	_KFIFO_TYPE(, TYPE, N) FIFO

#define	INIT_KFIFO(FIFO) do						      \
{									      \
	_init_kfifo(&(FIFO).kf_meta, sizeof((FIFO).kf_buf));		      \
} while (0)

#define	FINI_KFIFO(FIFO) do						      \
{									      \
	_fini_kfifo(&(FIFO).kf_meta);					      \
} while (0)

static inline void
_init_kfifo(struct kfifo_meta *meta, size_t nbytes)
{

	mutex_init(&meta->kfm_lock, MUTEX_DEFAULT, IPL_VM);
	meta->kfm_head = 0;
	meta->kfm_tail = 0;
	meta->kfm_nbytes = nbytes;
}

static inline void
_fini_kfifo(struct kfifo_meta *meta)
{

	mutex_destroy(&meta->kfm_lock);
}

_KFIFO_PTR_TYPE(kfifo, unsigned char);

#define	kfifo_alloc(FIFO, SIZE, GFP)					      \
	_kfifo_alloc(&(FIFO)->kf_meta, &(FIFO)->kf_buf, (SIZE),		      \
	    sizeof(*(FIFO)->kf_buf), (GFP))

static inline int
_kfifo_alloc(struct kfifo_meta *meta, void *bufp, size_t size, size_t esize,
    gfp_t gfp)
{
	size_t nbytes;
	void *buf;

	if (size < 2 || esize == 0 || size > (size_t)-1/esize)
		return -EINVAL;
	nbytes = size*esize;

	buf = kmalloc(nbytes, gfp);
	if (buf == NULL)
		return -ENOMEM;

	/* Type pun!  Hope void * == struct whatever *.  */
	memcpy(bufp, &buf, sizeof(void *));

	_init_kfifo(meta, nbytes);

	return 0;
}

#define	kfifo_free(FIFO)						      \
	_kfifo_free(&(FIFO)->kf_meta, &(FIFO)->kf_buf)

static inline void
_kfifo_free(struct kfifo_meta *meta, void *bufp)
{
	void *buf;

	mutex_destroy(&meta->kfm_lock);

	memcpy(&buf, bufp, sizeof(void *));
	kfree(buf);

	/* Paranoia.  */
	buf = NULL;
	memcpy(bufp, &buf, sizeof(void *));
}

#define	kfifo_is_empty(FIFO)	(kfifo_len(FIFO) == 0)
#define	kfifo_is_full(FIFO)	(kfifo_avail(FIFO) == 0)
#define	kfifo_len(FIFO)							\
	(_kfifo_len(&(FIFO)->kf_meta)/sizeof(*(FIFO)->kf_buf))
#define	kfifo_size(FIFO)						\
	((FIFO)->kf_meta.kfm_nbytes/sizeof(*(FIFO)->kf_buf))
#define	kfifo_avail(FIFO)						\
	(_kfifo_avail(&(FIFO)->kf_meta)/sizeof(*(FIFO)->kf_buf))

static inline size_t
_kfifo_len(struct kfifo_meta *meta)
{
	size_t len;

	mutex_spin_enter(&meta->kfm_lock);
	len = meta->kfm_tail - meta->kfm_head;
	mutex_spin_exit(&meta->kfm_lock);

	return len;
}

static inline size_t
_kfifo_avail(struct kfifo_meta *meta)
{
	size_t avail;

	mutex_spin_enter(&meta->kfm_lock);
	avail = meta->kfm_nbytes - (meta->kfm_tail - meta->kfm_head);
	mutex_spin_exit(&meta->kfm_lock);

	return avail;
}

#define	kfifo_out_peek(FIFO, PTR, SIZE)					      \
	(_kfifo_out_peek(&(FIFO)->kf_meta, (FIFO)->kf_buf, (PTR),		      \
	    (SIZE)*sizeof(*(FIFO)->kf_buf))/sizeof(*(FIFO)->kf_buf))

static inline size_t
_kfifo_out_peek(struct kfifo_meta *meta, void *buf, void *ptr, size_t size)
{
	const char *src = buf;
	char *dst = ptr;
	size_t first, head, used;

	mutex_spin_enter(&meta->kfm_lock);
	used = meta->kfm_tail - meta->kfm_head;
	if (size > used)
		size = used;
	if (size != 0) {
		head = meta->kfm_head % meta->kfm_nbytes;
		first = meta->kfm_nbytes - head;
		if (first > size)
			first = size;
		memcpy(dst, src + head, first);
		memcpy(dst + first, src, size - first);
	}
	mutex_spin_exit(&meta->kfm_lock);

	return size;
}

#define	kfifo_out(FIFO, PTR, SIZE)					      \
	(_kfifo_out(&(FIFO)->kf_meta, (FIFO)->kf_buf, (PTR),		      \
	    (SIZE)*sizeof(*(FIFO)->kf_buf))/sizeof(*(FIFO)->kf_buf))

static inline size_t
_kfifo_out(struct kfifo_meta *meta, const void *buf, void *ptr, size_t size)
{
	const char *src = buf;
	char *dst = ptr;
	size_t first, head, used;

	mutex_spin_enter(&meta->kfm_lock);
	used = meta->kfm_tail - meta->kfm_head;
	if (size > used)
		size = used;
	if (size != 0) {
		head = meta->kfm_head % meta->kfm_nbytes;
		first = meta->kfm_nbytes - head;
		if (first > size)
			first = size;
		memcpy(dst, src + head, first);
		memcpy(dst + first, src, size - first);
		meta->kfm_head += size;
	}
	mutex_spin_exit(&meta->kfm_lock);

	return size;
}

#define	kfifo_in(FIFO, PTR, SIZE)					      \
	(_kfifo_in(&(FIFO)->kf_meta, (FIFO)->kf_buf, (PTR),		      \
	    (SIZE)*sizeof(*(FIFO)->kf_buf))/sizeof(*(FIFO)->kf_buf))

static inline size_t
_kfifo_in(struct kfifo_meta *meta, void *buf, const void *ptr, size_t size)
{
	const char *src = ptr;
	char *dst = buf;
	size_t avail, first, tail;

	mutex_spin_enter(&meta->kfm_lock);
	avail = meta->kfm_nbytes - (meta->kfm_tail - meta->kfm_head);
	if (size > avail)
		size = avail;
	if (size != 0) {
		tail = meta->kfm_tail % meta->kfm_nbytes;
		first = meta->kfm_nbytes - tail;
		if (first > size)
			first = size;
		memcpy(dst + tail, src, first);
		memcpy(dst, src + first, size - first);
		meta->kfm_tail += size;
	}
	mutex_spin_exit(&meta->kfm_lock);

	return size;
}

#define	kfifo_put(FIFO, VALUE)						\
({										\
	__typeof__(*(FIFO)->kf_buf) _kfifo_value = (VALUE);		      \
	kfifo_in((FIFO), &_kfifo_value, 1);				      \
})

#define	kfifo_get(FIFO, PTR)	kfifo_out((FIFO), (PTR), 1)
#define	kfifo_peek(FIFO, PTR)	kfifo_out_peek((FIFO), (PTR), 1)

#define	kfifo_skip_count(FIFO, SIZE) do				      \
{									      \
	(void)_kfifo_skip(&(FIFO)->kf_meta,				      \
	    (SIZE)*sizeof(*(FIFO)->kf_buf));				      \
} while (0)

static inline size_t
_kfifo_skip(struct kfifo_meta *meta, size_t size)
{
	size_t used;

	mutex_spin_enter(&meta->kfm_lock);
	used = meta->kfm_tail - meta->kfm_head;
	if (size > used)
		size = used;
	meta->kfm_head += size;
	mutex_spin_exit(&meta->kfm_lock);

	return size;
}

#define	kfifo_out_linear_ptr(FIFO, PTR, SIZE)				      \
({									      \
	size_t _kfifo_offset;						      \
	size_t _kfifo_size = _kfifo_out_linear(&(FIFO)->kf_meta,		      \
	    &_kfifo_offset, (SIZE)*sizeof(*(FIFO)->kf_buf));		      \
	*(PTR) = (__typeof__(*(PTR)))((char *)(FIFO)->kf_buf +		      \
	    _kfifo_offset);						      \
	_kfifo_size/sizeof(*(FIFO)->kf_buf);				      \
})

static inline size_t
_kfifo_out_linear(struct kfifo_meta *meta, size_t *offsetp, size_t size)
{
	size_t head, used;

	mutex_spin_enter(&meta->kfm_lock);
	used = meta->kfm_tail - meta->kfm_head;
	if (size > used)
		size = used;
	head = meta->kfm_head % meta->kfm_nbytes;
	if (size > meta->kfm_nbytes - head)
		size = meta->kfm_nbytes - head;
	*offsetp = head;
	mutex_spin_exit(&meta->kfm_lock);

	return size;
}

#endif	/* _LINUX_KFIFO_H_ */

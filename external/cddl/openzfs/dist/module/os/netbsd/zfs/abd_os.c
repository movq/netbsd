// SPDX-License-Identifier: CDDL-1.0
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or https://opensource.org/licenses/CDDL-1.0.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (c) 2014 by Chunwei Chen. All rights reserved.
 * Copyright (c) 2016, 2019 by Delphix. All rights reserved.
 * Copyright (c) 2023, 2024, Klara Inc.
 *
 * Adapted from the FreeBSD ABD implementation. Scatter buffers contain
 * permanently mapped, page-sized chunks supplied by the NetBSD pool cache.
 */

#include <sys/zfs_context.h>
#include <sys/abd_impl.h>
#include <sys/arc.h>
#include <sys/zio.h>

enum {
	ABD_STRUCT_SIZE,
	ABD_SCATTER_COUNT,
	ABD_SCATTER_SIZE,
	ABD_SCATTER_WASTE,
	ABD_LINEAR_COUNT,
	ABD_LINEAR_SIZE,
	ABD_NSTATS
};

static kstat_named_t abd_stats[ABD_NSTATS] = {
	{ "struct_size", KSTAT_DATA_UINT64 },
	{ "scatter_cnt", KSTAT_DATA_UINT64 },
	{ "scatter_data_size", KSTAT_DATA_UINT64 },
	{ "scatter_chunk_waste", KSTAT_DATA_UINT64 },
	{ "linear_cnt", KSTAT_DATA_UINT64 },
	{ "linear_data_size", KSTAT_DATA_UINT64 },
};
static wmsum_t abd_sums[ABD_NSTATS];
static kstat_t *abd_ksp;
static kmem_cache_t *abd_chunk_cache;
static void *abd_zero_chunk;
static size_t zfs_abd_scatter_min_size = PAGE_SIZE + 1;

abd_t *abd_zero_scatter;

static uint_t
abd_chunkcnt(size_t size)
{
	return (howmany(size, PAGE_SIZE));
}

static uint_t
abd_scatter_chunkcnt(abd_t *abd)
{
	ASSERT(!abd_is_linear(abd));
	return (abd_chunkcnt(ABD_SCATTER(abd).abd_offset + abd->abd_size));
}

static size_t
abd_struct_size(uint_t chunks)
{
	return (MAX(sizeof (abd_t),
	    offsetof(abd_t, abd_u.abd_scatter.abd_chunks) +
	    chunks * sizeof (void *)));
}

boolean_t
abd_size_alloc_linear(size_t size)
{
	return (!zfs_abd_scatter_enabled || size < zfs_abd_scatter_min_size);
}

void
abd_update_scatter_stats(abd_t *abd, abd_stats_op_t op)
{
	int64_t waste = (uint64_t)abd_scatter_chunkcnt(abd) * PAGE_SIZE -
	    abd->abd_size;
	int sign = op == ABDSTAT_INCR ? 1 : -1;

	ASSERT(op == ABDSTAT_INCR || op == ABDSTAT_DECR);
	wmsum_add(&abd_sums[ABD_SCATTER_COUNT], sign);
	wmsum_add(&abd_sums[ABD_SCATTER_SIZE], sign * (int64_t)abd->abd_size);
	wmsum_add(&abd_sums[ABD_SCATTER_WASTE], sign * waste);
	if (op == ABDSTAT_INCR)
		arc_space_consume(waste, ARC_SPACE_ABD_CHUNK_WASTE);
	else
		arc_space_return(waste, ARC_SPACE_ABD_CHUNK_WASTE);
}

void
abd_update_linear_stats(abd_t *abd, abd_stats_op_t op)
{
	int sign = op == ABDSTAT_INCR ? 1 : -1;

	ASSERT(op == ABDSTAT_INCR || op == ABDSTAT_DECR);
	wmsum_add(&abd_sums[ABD_LINEAR_COUNT], sign);
	wmsum_add(&abd_sums[ABD_LINEAR_SIZE], sign * (int64_t)abd->abd_size);
}

void
abd_verify_scatter(abd_t *abd)
{
	/* User-page ABDs require a separate mapping implementation. */
	VERIFY(!abd_is_from_pages(abd));
	VERIFY(!abd_is_linear_page(abd));
	ASSERT3U(ABD_SCATTER(abd).abd_offset, <, PAGE_SIZE);
	for (uint_t i = 0; i < abd_scatter_chunkcnt(abd); i++)
		ASSERT3P(ABD_SCATTER(abd).abd_chunks[i], !=, NULL);
}

void
abd_alloc_chunks(abd_t *abd, size_t size)
{
	for (uint_t i = 0; i < abd_chunkcnt(size); i++) {
		ABD_SCATTER(abd).abd_chunks[i] =
		    kmem_cache_alloc(abd_chunk_cache, KM_PUSHPAGE);
	}
}

void
abd_free_chunks(abd_t *abd)
{
	VERIFY(!abd_is_from_pages(abd));
	for (uint_t i = 0; i < abd_scatter_chunkcnt(abd); i++)
		kmem_cache_free(abd_chunk_cache, ABD_SCATTER(abd).abd_chunks[i]);
}

abd_t *
abd_alloc_struct_impl(size_t size)
{
	size_t bytes = abd_struct_size(abd_chunkcnt(size));
	abd_t *abd = kmem_alloc(bytes, KM_PUSHPAGE);

	wmsum_add(&abd_sums[ABD_STRUCT_SIZE], bytes);
	return (abd);
}

void
abd_free_struct_impl(abd_t *abd)
{
	uint_t chunks = abd_is_linear(abd) || abd_is_gang(abd) ? 0 :
	    abd_scatter_chunkcnt(abd);
	size_t bytes = abd_struct_size(chunks);

	kmem_free(abd, bytes);
	wmsum_add(&abd_sums[ABD_STRUCT_SIZE], -(int64_t)bytes);
}

static int
abd_kstats_update(kstat_t *ksp, int rw)
{
	kstat_named_t *stats = ksp->ks_data;

	if (rw == KSTAT_WRITE)
		return (EACCES);
	for (uint_t i = 0; i < ABD_NSTATS; i++)
		stats[i].value.ui64 = wmsum_value(&abd_sums[i]);
	return (0);
}

void
abd_init(void)
{
	abd_chunk_cache = kmem_cache_create("abd_chunk", PAGE_SIZE, PAGE_SIZE,
	    NULL, NULL, NULL, NULL, NULL, KMC_NODEBUG);
	for (uint_t i = 0; i < ABD_NSTATS; i++)
		wmsum_init(&abd_sums[i], 0);

	abd_ksp = kstat_create("zfs", 0, "abdstats", "misc", KSTAT_TYPE_NAMED,
	    ABD_NSTATS, KSTAT_FLAG_VIRTUAL);
	if (abd_ksp != NULL) {
		abd_ksp->ks_data = abd_stats;
		abd_ksp->ks_update = abd_kstats_update;
		kstat_install(abd_ksp);
	}

	/* Share one zeroed chunk across a maximum-sized scatter ABD. */
	abd_zero_chunk = kmem_cache_alloc(abd_chunk_cache, KM_PUSHPAGE);
	memset(abd_zero_chunk, 0, PAGE_SIZE);
	abd_zero_scatter = abd_alloc_struct(SPA_MAXBLOCKSIZE);
	abd_zero_scatter->abd_flags |= ABD_FLAG_OWNER;
	abd_zero_scatter->abd_size = SPA_MAXBLOCKSIZE;
	ABD_SCATTER(abd_zero_scatter).abd_offset = 0;
	for (uint_t i = 0; i < abd_chunkcnt(SPA_MAXBLOCKSIZE); i++)
		ABD_SCATTER(abd_zero_scatter).abd_chunks[i] = abd_zero_chunk;
	wmsum_add(&abd_sums[ABD_SCATTER_COUNT], 1);
	wmsum_add(&abd_sums[ABD_SCATTER_SIZE], PAGE_SIZE);
}

void
abd_fini(void)
{
	wmsum_add(&abd_sums[ABD_SCATTER_COUNT], -1);
	wmsum_add(&abd_sums[ABD_SCATTER_SIZE], -(int64_t)PAGE_SIZE);
	abd_free_struct(abd_zero_scatter);
	abd_zero_scatter = NULL;
	kmem_cache_free(abd_chunk_cache, abd_zero_chunk);
	abd_zero_chunk = NULL;
	if (abd_ksp != NULL) {
		kstat_delete(abd_ksp);
		abd_ksp = NULL;
	}
	for (uint_t i = 0; i < ABD_NSTATS; i++)
		wmsum_fini(&abd_sums[i]);
	kmem_cache_destroy(abd_chunk_cache);
	abd_chunk_cache = NULL;
}

void
abd_free_linear_page(abd_t *abd)
{
	panic("%s: user-page ABDs are not implemented", __func__);
}

abd_t *
abd_alloc_for_io(size_t size, boolean_t is_metadata)
{
	/* NetBSD strategy buffers require a virtually contiguous address. */
	return (abd_alloc_linear(size, is_metadata));
}

abd_t *
abd_get_offset_scatter(abd_t *abd, abd_t *sabd, size_t off, size_t size)
{
	abd_verify(sabd);
	ASSERT3U(off + size, <=, sabd->abd_size);
	size_t offset = ABD_SCATTER(sabd).abd_offset + off;
	size_t chunks = abd_chunkcnt((offset & PAGE_MASK) + size);

	if (abd != NULL && abd_struct_size(chunks) > sizeof (*abd))
		abd = NULL;
	if (abd == NULL)
		abd = abd_alloc_struct(chunks * PAGE_SIZE);
	ABD_SCATTER(abd).abd_offset = offset & PAGE_MASK;
	memcpy(ABD_SCATTER(abd).abd_chunks,
	    &ABD_SCATTER(sabd).abd_chunks[offset >> PAGE_SHIFT],
	    chunks * sizeof (void *));
	return (abd);
}

void
abd_iter_init(struct abd_iter *iter, abd_t *abd)
{
	ASSERT(!abd_is_gang(abd));
	abd_verify(abd);
	memset(iter, 0, sizeof (*iter));
	iter->iter_abd = abd;
}

boolean_t
abd_iter_at_end(struct abd_iter *iter)
{
	return (iter->iter_pos == iter->iter_abd->abd_size);
}

void
abd_iter_advance(struct abd_iter *iter, size_t amount)
{
	ASSERT0P(iter->iter_mapaddr);
	ASSERT0(iter->iter_mapsize);
	if (!abd_iter_at_end(iter))
		iter->iter_pos += amount;
}

void
abd_iter_map(struct abd_iter *iter)
{
	abd_t *abd = iter->iter_abd;
	size_t offset = iter->iter_pos;
	void *base;

	ASSERT0P(iter->iter_mapaddr);
	ASSERT0(iter->iter_mapsize);
	if (abd_iter_at_end(iter))
		return;
	VERIFY(!abd_is_from_pages(abd));
	if (abd_is_linear(abd)) {
		base = ABD_LINEAR_BUF(abd);
		iter->iter_mapsize = abd->abd_size - offset;
	} else {
		offset += ABD_SCATTER(abd).abd_offset;
		base = ABD_SCATTER(abd).abd_chunks[offset >> PAGE_SHIFT];
		offset &= PAGE_MASK;
		iter->iter_mapsize = MIN(PAGE_SIZE - offset,
		    abd->abd_size - iter->iter_pos);
	}
	iter->iter_mapaddr = (char *)base + offset;
}

void
abd_iter_unmap(struct abd_iter *iter)
{
	iter->iter_mapaddr = NULL;
	iter->iter_mapsize = 0;
}

void
abd_cache_reap_now(void)
{
	kmem_cache_reap_now(abd_chunk_cache);
}

void *
abd_borrow_buf(abd_t *abd, size_t size)
{
	abd_verify(abd);
	ASSERT3U(size, <=, abd->abd_size);
	void *buf = abd_is_linear(abd) ? abd_to_buf(abd) : zio_buf_alloc(size);
#ifdef ZFS_DEBUG
	(void) zfs_refcount_add_many(&abd->abd_children, size, buf);
#endif
	return (buf);
}

void *
abd_borrow_buf_copy(abd_t *abd, size_t size)
{
	void *buf = abd_borrow_buf(abd, size);

	if (!abd_is_linear(abd))
		abd_copy_to_buf(buf, abd, size);
	return (buf);
}

void
abd_return_buf(abd_t *abd, void *buf, size_t size)
{
	abd_verify(abd);
	ASSERT3U(size, <=, abd->abd_size);
#ifdef ZFS_DEBUG
	(void) zfs_refcount_remove_many(&abd->abd_children, size, buf);
#endif
	if (abd_is_linear(abd)) {
		ASSERT3P(buf, ==, abd_to_buf(abd));
	} else {
		ASSERT0(abd_cmp_buf(abd, buf, size));
		zio_buf_free(buf, size);
	}
}

void
abd_return_buf_copy(abd_t *abd, void *buf, size_t size)
{
	if (!abd_is_linear(abd))
		abd_copy_from_buf(abd, buf, size);
	abd_return_buf(abd, buf, size);
}

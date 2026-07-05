/*	$NetBSD$	*/

/*
 * NetBSD native backing for the Linux 6.18 TTM pool interface.
 *
 * Linux pools pages primarily to amortize cache-attribute transitions.
 * NetBSD instead keeps each TT's pageable contents in a UAO, wires it while
 * the GPU can access it, and describes the wired pages with one bus_dma map.
 * UVM remains responsible for reclaim and swap.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/bus.h>
#include <sys/errno.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

#include <linux/export.h>
#include <linux/slab.h>

#include <drm/bus_dma_hacks.h>
#include <drm/ttm/ttm_pool.h>
#include <drm/ttm/ttm_tt.h>

static void
ttm_pool_clear_pages(struct ttm_tt *tt)
{
	unsigned int i;

	for (i = 0; i < tt->num_pages; i++) {
		tt->pages[i] = NULL;
		if (tt->dma_address)
			tt->dma_address[i] = 0;
	}
}

static int
ttm_pool_load_dma(struct ttm_tt *tt)
{
	const bus_size_t size = (bus_size_t)tt->num_pages << PAGE_SHIFT;
	unsigned int page = 0;
	int error, seg;

	if (tt->dma_address == NULL)
		return 0;

	error = bus_dmamap_create(tt->dmat, size, tt->num_pages, size, 0,
	    BUS_DMA_WAITOK, &tt->dma_map);
	if (error)
		return -error;

	error = bus_dmamap_load_pages(tt->dmat, tt->dma_map, tt->pages, size,
	    BUS_DMA_WAITOK);
	if (error)
		goto fail;

	for (seg = 0; seg < tt->dma_map->dm_nsegs; seg++) {
		bus_addr_t addr = tt->dma_map->dm_segs[seg].ds_addr;
		bus_size_t len = tt->dma_map->dm_segs[seg].ds_len;

		while (len != 0) {
			if (len < PAGE_SIZE || page >= tt->num_pages) {
				error = EFBIG;
				goto fail_unload;
			}
			tt->dma_address[page++] = addr;
			addr += PAGE_SIZE;
			len -= PAGE_SIZE;
		}
	}
	if (page != tt->num_pages) {
		error = EFBIG;
		goto fail_unload;
	}

	return 0;

fail_unload:
	bus_dmamap_unload(tt->dmat, tt->dma_map);
fail:
	bus_dmamap_destroy(tt->dmat, tt->dma_map);
	tt->dma_map = NULL;
	return -error;
}

int
ttm_pool_alloc(struct ttm_pool *pool, struct ttm_tt *tt,
    struct ttm_operation_ctx *ctx)
{
	const voff_t size = (voff_t)tt->num_pages << PAGE_SHIFT;
	struct pglist pages;
	struct vm_page *pg;
	unsigned int i;
	int error;

	KASSERT(tt->swap_storage != NULL);
	KASSERT(tt->dma_map == NULL);
	TAILQ_INIT(&pages);

	error = uvm_obj_wirepages(tt->swap_storage, 0, size, &pages);
	if (error)
		return -error;

	for (i = 0; i < tt->num_pages; i++) {
		pg = TAILQ_FIRST(&pages);
		KASSERT(pg != NULL);
		TAILQ_REMOVE(&pages, pg, pageq.queue);
		tt->pages[i] = container_of(pg, struct page, p_vmp);
		if (tt->page_flags & TTM_TT_FLAG_ZERO_ALLOC)
			uvm_pagezero(pg);
	}
	KASSERT(TAILQ_EMPTY(&pages));

	error = ttm_pool_load_dma(tt);
	if (error) {
		uvm_obj_unwirepages(tt->swap_storage, 0, size);
		ttm_pool_clear_pages(tt);
		return error;
	}

	tt->page_flags &= ~TTM_TT_FLAG_ZERO_ALLOC;
	return 0;
}
EXPORT_SYMBOL(ttm_pool_alloc);

void
ttm_pool_free(struct ttm_pool *pool, struct ttm_tt *tt)
{
	const voff_t size = (voff_t)tt->num_pages << PAGE_SHIFT;
	struct uvm_object *uobj = tt->swap_storage;

	if (tt->dma_map != NULL) {
		bus_dmamap_unload(tt->dmat, tt->dma_map);
		bus_dmamap_destroy(tt->dmat, tt->dma_map);
		tt->dma_map = NULL;
	}

	if (tt->num_pages != 0 && tt->pages[0] != NULL) {
		uvm_obj_unwirepages(uobj, 0, size);
		rw_enter(uobj->vmobjlock, RW_WRITER);
		(void)(*uobj->pgops->pgo_put)(uobj, 0, size, PGO_DEACTIVATE);
		/* pgo_put releases uobj->vmobjlock. */
	}
	ttm_pool_clear_pages(tt);
}
EXPORT_SYMBOL(ttm_pool_free);

void
ttm_pool_init(struct ttm_pool *pool, struct device *dev, int nid,
    bool use_dma_alloc, bool use_dma32)
{
	pool->dev = dev;
	pool->nid = nid;
	pool->use_dma_alloc = use_dma_alloc;
	pool->use_dma32 = use_dma32;
}

void
ttm_pool_fini(struct ttm_pool *pool)
{
}

int
ttm_pool_debugfs(struct ttm_pool *pool, struct seq_file *m)
{
	return 0;
}

void
ttm_pool_drop_backed_up(struct ttm_tt *tt)
{
	kfree(tt->restore);
	tt->restore = NULL;
}

long
ttm_pool_backup(struct ttm_pool *pool, struct ttm_tt *tt,
    const struct ttm_backup_flags *flags)
{
	ttm_pool_free(pool, tt);
	return tt->num_pages;
}

int
ttm_pool_restore_and_alloc(struct ttm_pool *pool, struct ttm_tt *tt,
    const struct ttm_operation_ctx *ctx)
{
	return ttm_pool_alloc(pool, tt, __UNCONST(ctx));
}

int
ttm_pool_mgr_init(unsigned long num_pages)
{
	return 0;
}

void
ttm_pool_mgr_fini(void)
{
}

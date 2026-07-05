/*	$NetBSD$	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

/*
 * NetBSD UVM pager for the Linux 6.18 TTM resource model.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/bus.h>
#include <sys/errno.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>
#include <uvm/uvm_fault.h>

#include <drm/drm_gem.h>
#include <drm/ttm/ttm_bo.h>
#include <drm/ttm/ttm_device.h>
#include <drm/ttm/ttm_tt.h>

static struct ttm_buffer_object *
ttm_bo_from_uobj(struct uvm_object *uobj)
{
	struct drm_gem_object *obj = container_of(uobj,
	    struct drm_gem_object, gemo_uvmobj);

	return container_of(obj, struct ttm_buffer_object, base);
}

void
ttm_bo_uvm_reference(struct uvm_object *uobj)
{
	drm_gem_object_get(&ttm_bo_from_uobj(uobj)->base);
}

void
ttm_bo_uvm_detach(struct uvm_object *uobj)
{
	drm_gem_object_put(&ttm_bo_from_uobj(uobj)->base);
}

int
ttm_bo_uvm_reserve(struct ttm_buffer_object *bo, struct uvm_faultinfo *ufi)
{
	if (__predict_false(!dma_resv_trylock(bo->base.resv))) {
		drm_gem_object_get(&bo->base);
		uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap, NULL);
		if (!dma_resv_lock_interruptible(bo->base.resv, NULL))
			dma_resv_unlock(bo->base.resv);
		drm_gem_object_put(&bo->base);
		return ERESTART;
	}

	if (bo->ttm != NULL &&
	    (bo->ttm->page_flags & TTM_TT_FLAG_EXTERNAL) != 0 &&
	    (bo->ttm->page_flags & TTM_TT_FLAG_EXTERNAL_MAPPABLE) == 0) {
		dma_resv_unlock(bo->base.resv);
		return EINVAL;
	}

	return 0;
}

static int
ttm_bo_vm_fault_idle(struct ttm_buffer_object *bo, struct uvm_faultinfo *ufi)
{
	long error;

	if (dma_resv_test_signaled(bo->base.resv, DMA_RESV_USAGE_KERNEL))
		return 0;

	drm_gem_object_get(&bo->base);
	uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap, NULL);
	error = dma_resv_wait_timeout(bo->base.resv, DMA_RESV_USAGE_KERNEL,
	    true, MAX_SCHEDULE_TIMEOUT);
	dma_resv_unlock(bo->base.resv);
	drm_gem_object_put(&bo->base);

	if (error < 0 && error != -ERESTARTSYS)
		return EINVAL;
	return ERESTART;
}

int
ttm_bo_uvm_fault_reserved(struct uvm_faultinfo *ufi, vaddr_t vaddr,
    struct vm_page **pps, int npages, int centeridx, vm_prot_t access_type,
    int flags)
{
	struct uvm_object *uobj = ufi->entry->object.uvm_obj;
	struct ttm_buffer_object *bo = ttm_bo_from_uobj(uobj);
	struct ttm_device *bdev = bo->bdev;
	struct ttm_resource *res = bo->resource;
	struct ttm_tt *ttm = NULL;
	voff_t offset;
	unsigned long startpage;
	vm_prot_t vm_prot = ufi->entry->protection;
	pgprot_t prot;
	int error, i;

	error = ttm_bo_vm_fault_idle(bo, ufi);
	if (error)
		return error;

	if (res == NULL)
		return EINVAL;
	error = ttm_mem_io_reserve(bdev, res);
	if (error)
		return EINVAL;

	KASSERT(ufi->entry->start <= vaddr);
	offset = ufi->entry->offset + (vaddr - ufi->entry->start);
	if ((offset & PAGE_MASK) >= bo->base.size)
		return EINVAL;
	startpage = offset >> PAGE_SHIFT;

	prot = ttm_io_prot(bo, res, vm_prot);
	if (!res->bus.is_iomem) {
		struct ttm_operation_ctx ctx = {
			.interruptible = true,
			.no_wait_gpu = false,
		};

		error = ttm_bo_populate(bo, &ctx);
		if (error)
			return (error == -ERESTARTSYS || error == -EAGAIN) ?
			    0 : ENOMEM;
		ttm = bo->ttm;
	}

	for (i = 0; i < npages; i++) {
		unsigned long page = startpage + i;
		paddr_t paddr;

		if ((flags & PGO_ALLPAGES) == 0 && i != centeridx)
			continue;
		if (pps[i] == PGO_DONTCARE)
			continue;
		if ((page << PAGE_SHIFT) >= bo->base.size)
			break;

		if (!res->bus.is_iomem) {
			if (ttm == NULL || ttm->pages[page] == NULL)
				return ENOMEM;
			paddr = page_to_phys(ttm->pages[page]);
		} else if (bdev->funcs->io_mem_pfn != NULL) {
			paddr = (paddr_t)bdev->funcs->io_mem_pfn(bo, page)
			    << PAGE_SHIFT;
		} else {
			paddr_t cookie = bus_space_mmap(bdev->memt,
			    res->bus.offset, (off_t)page << PAGE_SHIFT,
			    vm_prot, 0);

			paddr = pmap_phys_address(cookie);
		}

		error = pmap_enter(ufi->orig_map->pmap,
		    vaddr + (vaddr_t)i * PAGE_SIZE, paddr, vm_prot,
		    PMAP_CANFAIL | prot);
		if (error != 0 && i == centeridx)
			return ENOMEM;
	}
	pmap_update(ufi->orig_map->pmap);
	return 0;
}

int
ttm_bo_uvm_fault(struct uvm_faultinfo *ufi, vaddr_t vaddr,
    struct vm_page **pps, int npages, int centeridx, vm_prot_t access_type,
    int flags)
{
	struct uvm_object *uobj = ufi->entry->object.uvm_obj;
	struct ttm_buffer_object *bo = ttm_bo_from_uobj(uobj);
	int error;

	/* UVM enters with the object lock held; TTM uses the reservation. */
	rw_exit(uobj->vmobjlock);

	if (UVM_ET_ISCOPYONWRITE(ufi->entry)) {
		error = EINVAL;
		goto out;
	}

	error = ttm_bo_uvm_reserve(bo, ufi);
	if (error != 0) {
		if (error == ERESTART)
			return error;
		goto out;
	}

	error = ttm_bo_uvm_fault_reserved(ufi, vaddr, pps, npages, centeridx,
	    access_type, flags);
	if (error == ERESTART)
		return error;

	dma_resv_unlock(bo->base.resv);
out:
	uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap, NULL);
	return error;
}

const struct uvm_pagerops ttm_bo_uvm_ops = {
	.pgo_reference = ttm_bo_uvm_reference,
	.pgo_detach = ttm_bo_uvm_detach,
	.pgo_fault = ttm_bo_uvm_fault,
};

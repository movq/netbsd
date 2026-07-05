/*	$NetBSD: drm_memory.c,v 1.17 2021/12/19 10:47:13 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_memory.c,v 1.17 2021/12/19 10:47:13 riastradh Exp $");

#include <sys/bus.h>

#include <drm/drm_device.h>

/*
 * Make sure the DMA-safe memory allocated for dev lies between
 * min_addr and max_addr.  Can be used multiple times to restrict the
 * bounds further, but never to expand the bounds again.
 *
 * XXX Caller must guarantee nobody has used the tag yet,
 * i.e. allocated any DMA memory.
 */
int
drm_limit_dma_space(struct drm_device *dev, resource_size_t min_addr,
    resource_size_t max_addr)
{
	int ret;

	KASSERT(min_addr <= max_addr);

	/*
	 * Limit it further if we have already limited it, and destroy
	 * the old subregion DMA tag.
	 */
	if (dev->dmat_subregion_p) {
		min_addr = MAX(min_addr, dev->dmat_subregion_min);
		max_addr = MIN(max_addr, dev->dmat_subregion_max);
		bus_dmatag_destroy(dev->dmat);
	}

	/*
	 * If our limit contains the 32-bit space but for some reason
	 * we can't use a subregion, either because the bus doesn't
	 * support >32-bit DMA or because bus_dma(9) on this platform
	 * lacks bus_dmatag_subregion, just use the 32-bit space.
	 */
	if (min_addr == 0 && max_addr >= UINT32_C(0xffffffff) &&
	    dev->bus_dmat == dev->bus_dmat32) {
dma32:		dev->dmat = dev->bus_dmat32;
		dev->dmat_subregion_p = false;
		dev->dmat_subregion_min = 0;
		dev->dmat_subregion_max = UINT32_C(0xffffffff);
		return 0;
	}

	/*
	 * Create a DMA tag for a subregion from the bus's DMA tag.  If
	 * that fails, restore dev->dmat to the whole region so that we
	 * need not worry about dev->dmat being uninitialized (not that
	 * the caller should try to allocate DMA-safe memory on failure
	 * anyway, but...paranoia).
	 */
	/* XXX errno NetBSD->Linux */
	ret = -bus_dmatag_subregion(dev->bus_dmat, min_addr, max_addr,
	    &dev->dmat, BUS_DMA_WAITOK);
	if (ret) {
		/*
		 * bus_dmatag_subregion may fail.  If so, and if the
		 * subregion contains the 32-bit space, just use the
		 * 32-bit DMA tag.
		 */
		if (ret == -EOPNOTSUPP && dev->bus_dmat32 &&
		    min_addr == 0 && max_addr >= UINT32_C(0xffffffff))
			goto dma32;
		/* XXX Back out?  */
		dev->dmat = dev->bus_dmat;
		dev->dmat_subregion_p = false;
		dev->dmat_subregion_min = 0;
		dev->dmat_subregion_max = __type_max(bus_addr_t);
		return ret;
	}

	/*
	 * Remember that we have a subregion tag so that we know to
	 * destroy it later, and record the bounds in case we need to
	 * limit them again.
	 */
	dev->dmat_subregion_p = true;
	dev->dmat_subregion_min = min_addr;
	dev->dmat_subregion_max = max_addr;

	/* Success!  */
	return 0;
}

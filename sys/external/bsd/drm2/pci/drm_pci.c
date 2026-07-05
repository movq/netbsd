/*	$NetBSD: drm_pci.c,v 1.48 2022/10/28 21:58:48 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_pci.c,v 1.48 2022/10/28 21:58:48 riastradh Exp $");

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>

#include <linux/err.h>
#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_pci.h>

#include "../dist/drm/drm_internal.h"

/*
 * NetBSD records PCI BARs here so Linux bus-space accessors can map them
 * lazily.
 */
struct drm_bus_map {
	bus_addr_t bm_base;
	bus_size_t bm_size;
	bus_space_handle_t bm_bsh;
	int bm_flags;
};

struct drm_pci_irq {
	struct drm_device *dev;
	pci_intr_handle_t *intr_handles;
	void *ih_cookie;
	int (*handler)(void *);
	void *arg;
	kmutex_t lock;
	unsigned int disable_depth;
	bool msi;
};

static const struct pci_attach_args *
drm_pci_attach_args(struct drm_device *dev)
{
	return &dev->pdev->pd_pa;
}

int
drm_pci_attach(struct drm_device *dev, struct pci_dev *pdev)
{
	device_t self = dev->dev;
	const struct pci_attach_args *pa = &pdev->pd_pa;
	unsigned int unit;
	int ret;

	/* Ensure the drm agp hooks are initialized.  */
	/* XXX errno NetBSD->Linux */
	ret = -drm_guarantee_initialized();
	if (ret)
		return ret;

	dev->pdev = pdev;

	/* XXX Set the power state to D0?  */

	/* Set up the bus space and bus DMA tags.  */
	dev->bst = pa->pa_memt;
	dev->bus_dmat = (pci_dma64_available(pa)? pa->pa_dmat64 : pa->pa_dmat);
	dev->bus_dmat32 = pa->pa_dmat;
	dev->dmat = dev->bus_dmat;
	dev->dmat_subregion_p = false;
	dev->dmat_subregion_min = 0;
	dev->dmat_subregion_max = __type_max(bus_addr_t);

	/* Find all the memory maps.  */
	CTASSERT(PCI_NUM_RESOURCES < (SIZE_MAX / sizeof(dev->bus_maps[0])));
	dev->bus_maps = kmem_zalloc(PCI_NUM_RESOURCES *
	    sizeof(dev->bus_maps[0]), KM_SLEEP);
	dev->bus_nmaps = PCI_NUM_RESOURCES;
	for (unit = 0; unit < PCI_NUM_RESOURCES; unit++) {
		struct drm_bus_map *const bm = &dev->bus_maps[unit];
		const int reg = PCI_BAR(unit);
		const pcireg_t type =
		    pci_mapreg_type(pa->pa_pc, pa->pa_tag, reg);

		/* Reject non-memory mappings.  */
		if ((type & PCI_MAPREG_TYPE_MEM) != PCI_MAPREG_TYPE_MEM) {
			aprint_debug_dev(self, "map %u has non-memory type:"
			    " 0x%"PRIxMAX"\n", unit, (uintmax_t)type);
			continue;
		}

		/*
		 * If it's a 64-bit mapping, don't interpret the second
		 * half of it as another BAR in the next iteration of
		 * the loop -- move on to the next unit.
		 */
		if (PCI_MAPREG_MEM_TYPE(type) == PCI_MAPREG_MEM_TYPE_64BIT)
			unit++;

		/* Inquire about it.  We'll map it in drm_legacy_ioremap.  */
		if (pci_mapreg_info(pa->pa_pc, pa->pa_tag, reg, type,
			&bm->bm_base, &bm->bm_size, &bm->bm_flags) != 0) {
			aprint_debug_dev(self, "map %u failed\n", unit);
			continue;
		}

		/* Assume since it is a memory mapping it can be linear.  */
		bm->bm_flags |= BUS_SPACE_MAP_LINEAR;
	}

	/* Success!  */
	return 0;
}

void
drm_pci_detach(struct drm_device *dev)
{

	/* Free the record of available bus space mappings.  */
	dev->bus_nmaps = 0;
	kmem_free(dev->bus_maps, PCI_NUM_RESOURCES * sizeof(dev->bus_maps[0]));

	/* Tear down bus space and bus DMA tags.  */
	if (dev->dmat_subregion_p) {
		bus_dmatag_destroy(dev->dmat);
	}
}

static void *
drm_pci_irq_establish(struct drm_pci_irq *irq)
{
	struct drm_device *const dev = irq->dev;
	const char *const name = device_xname(dev->dev);
	const struct pci_attach_args *const pa = drm_pci_attach_args(dev);

	return pci_intr_establish_xname(pa->pa_pc, irq->intr_handles[0],
	    IPL_DRM, irq->handler, irq->arg, name);
}

int
drm_pci_irq_install(struct drm_device *dev, bool allow_msi,
    int (*handler)(void *), void *arg, struct drm_pci_irq **irqp, bool *msip)
{
	const char *const name = device_xname(dev->dev);
	const struct pci_attach_args *const pa = drm_pci_attach_args(dev);
	struct drm_pci_irq *irq;
	const char *intrstr;
	char intrbuf[PCI_INTRSTR_LEN];
	int error;

	KASSERT(*irqp == NULL);

	irq = kmem_zalloc(sizeof(*irq), KM_SLEEP);
	irq->dev = dev;
	irq->handler = handler;
	irq->arg = arg;
	mutex_init(&irq->lock, MUTEX_DEFAULT, IPL_NONE);

	if (allow_msi &&
	    pci_msi_alloc_exact(pa, &irq->intr_handles, 1) == 0) {
		irq->msi = true;
	} else if (pci_intx_alloc(pa, &irq->intr_handles) != 0) {
		aprint_error_dev(dev->dev,
		    "couldn't allocate PCI interrupt (%s)\n", name);
		error = -ENOENT;
		goto fail;
	}

	pci_intr_setattr(pa->pa_pc, &irq->intr_handles[0],
	    PCI_INTR_MPSAFE, true);
	intrstr = pci_intr_string(pa->pa_pc, irq->intr_handles[0],
	    intrbuf, sizeof(intrbuf));
	irq->ih_cookie = drm_pci_irq_establish(irq);
	if (irq->ih_cookie == NULL) {
		aprint_error_dev(dev->dev,
		    "couldn't establish interrupt at %s (%s)\n", intrstr, name);
		error = -ENOENT;
		goto fail;
	}

	dev->pdev->msi_enabled = irq->msi;
	aprint_normal_dev(dev->dev, "interrupting at %s (%s)\n", intrstr, name);
	*irqp = irq;
	*msip = irq->msi;
	return 0;

fail:
	if (irq->intr_handles != NULL)
		pci_intr_release(pa->pa_pc, irq->intr_handles, 1);
	mutex_destroy(&irq->lock);
	kmem_free(irq, sizeof(*irq));
	return error;
}

void
drm_pci_irq_uninstall(struct drm_pci_irq *irq)
{
	struct drm_device *const dev = irq->dev;
	const struct pci_attach_args *pa = drm_pci_attach_args(dev);

	if (irq->ih_cookie != NULL)
		pci_intr_disestablish(pa->pa_pc, irq->ih_cookie);
	pci_intr_release(pa->pa_pc, irq->intr_handles, 1);
	dev->pdev->msi_enabled = false;
	mutex_destroy(&irq->lock);
	kmem_free(irq, sizeof(*irq));
}

void
drm_pci_irq_disable(struct drm_pci_irq *irq)
{
	const struct pci_attach_args *const pa = drm_pci_attach_args(irq->dev);

	mutex_enter(&irq->lock);
	if (irq->disable_depth++ == 0) {
		KASSERT(irq->ih_cookie != NULL);
		pci_intr_disestablish(pa->pa_pc, irq->ih_cookie);
		irq->ih_cookie = NULL;
	}
	mutex_exit(&irq->lock);
}

void
drm_pci_irq_enable(struct drm_pci_irq *irq)
{

	mutex_enter(&irq->lock);
	KASSERT(irq->disable_depth != 0);
	if (--irq->disable_depth == 0) {
		KASSERT(irq->ih_cookie == NULL);
		irq->ih_cookie = drm_pci_irq_establish(irq);
		if (irq->ih_cookie == NULL)
			aprint_error_dev(irq->dev->dev,
			    "couldn't re-establish interrupt\n");
	}
	mutex_exit(&irq->lock);
}

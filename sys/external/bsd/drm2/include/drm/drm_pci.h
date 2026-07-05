/*	$NetBSD$	*/

/*
 * NetBSD PCI integration retained outside the imported Linux DRM tree.
 * Linux removed this header after moving its remaining legacy helpers.
 */

#ifndef _DRM_PCI_H_
#define _DRM_PCI_H_

#include <linux/pci.h>

struct drm_device;
struct drm_dma_handle;
struct drm_pci_irq;

struct drm_dma_handle *drm_pci_alloc(struct drm_device *, size_t, size_t);
void drm_pci_free(struct drm_device *, struct drm_dma_handle *);

int drm_pci_irq_install(struct drm_device *, bool, int (*)(void *), void *,
    struct drm_pci_irq **, bool *);
void drm_pci_irq_uninstall(struct drm_pci_irq *);
void drm_pci_irq_disable(struct drm_pci_irq *);
void drm_pci_irq_enable(struct drm_pci_irq *);
int drm_pci_attach(struct drm_device *, struct pci_dev *);
void drm_pci_detach(struct drm_device *);

#endif /* _DRM_PCI_H_ */

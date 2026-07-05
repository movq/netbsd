/*	$NetBSD$	*/

/*
 * NetBSD PCI interrupt integration for amdgpu.
 */

#ifndef _DRM_AMDGPU_PCI_H_
#define _DRM_AMDGPU_PCI_H_

#include <sys/types.h>

struct drm_device;

int	amdgpu_pci_irq_install(struct drm_device *, bool, int (*)(void *),
	    bool *);
void	amdgpu_pci_irq_uninstall(struct drm_device *);
void	amdgpu_pci_irq_disable(struct drm_device *);
void	amdgpu_pci_irq_enable(struct drm_device *);

#endif /* _DRM_AMDGPU_PCI_H_ */

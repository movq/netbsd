/*	$NetBSD$	*/

/* SPDX-License-Identifier: MIT */

#ifndef _LINUX_APERTURE_H_
#define _LINUX_APERTURE_H_

#include <linux/types.h>

struct pci_dev;
struct platform_device;

/*
 * NetBSD resolves framebuffer attachment through autoconfiguration match
 * priorities, so there is no conflicting firmware framebuffer to remove.
 */
static inline int
devm_aperture_acquire_for_platform_device(struct platform_device *pdev,
    resource_size_t base, resource_size_t size)
{

	return 0;
}

static inline int
aperture_remove_conflicting_devices(resource_size_t base,
    resource_size_t size, const char *name)
{

	return 0;
}

static inline int
__aperture_remove_legacy_vga_devices(struct pci_dev *pdev)
{

	return 0;
}

static inline int
aperture_remove_conflicting_pci_devices(struct pci_dev *pdev,
    const char *name)
{

	return 0;
}

static inline int
aperture_remove_all_conflicting_devices(const char *name)
{

	return 0;
}

#endif /* _LINUX_APERTURE_H_ */

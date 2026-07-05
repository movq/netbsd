/*	$NetBSD$	*/

/*
 * PCI peer-to-peer DMA is not supported by the NetBSD Linux compatibility
 * layer.  Report devices as unreachable so imported drivers use their
 * system-memory fallback.
 */

#ifndef _LINUX_PCI_P2PDMA_H_
#define _LINUX_PCI_P2PDMA_H_

#include <linux/types.h>

struct device;
struct pci_dev;

static inline int
pci_p2pdma_distance(struct pci_dev *provider, const void *client,
    bool verbose)
{

	return -1;
}

#endif /* _LINUX_PCI_P2PDMA_H_ */

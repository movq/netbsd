/*	$NetBSD$	*/

/* Public domain. */

#ifndef _LINUX_DMA_DIRECTION_H_
#define _LINUX_DMA_DIRECTION_H_

enum dma_data_direction {
	DMA_NONE		= 0,
	DMA_TO_DEVICE		= 1,
	DMA_FROM_DEVICE		= 2,
	DMA_BIDIRECTIONAL	= 3,
};

static inline int
valid_dma_direction(enum dma_data_direction direction)
{

	return direction == DMA_BIDIRECTIONAL ||
	    direction == DMA_TO_DEVICE ||
	    direction == DMA_FROM_DEVICE;
}

#endif	/* _LINUX_DMA_DIRECTION_H_ */

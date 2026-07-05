/*	$NetBSD$	*/

/* Public domain. */

#ifndef _LINUX_MEMREMAP_H_
#define _LINUX_MEMREMAP_H_

/*
 * Device-private memory is only used by the optional HSA/KFD support,
 * which is not enabled in NetBSD.
 */
struct dev_pagemap {
};

#endif	/* _LINUX_MEMREMAP_H_ */

/*	$NetBSD$	*/

/* Public domain. */

#ifndef _LINUX_MMZONE_H_
#define _LINUX_MMZONE_H_

#include <linux/mm_types.h>

#define	MAX_PAGE_ORDER	10
#define	NR_PAGE_ORDERS	(MAX_PAGE_ORDER + 1)

#define	pfn_to_nid(pfn)	0

#endif	/* _LINUX_MMZONE_H_ */

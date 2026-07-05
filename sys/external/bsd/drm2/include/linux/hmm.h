/*	$NetBSD$	*/

#ifndef _LINUX_HMM_H_
#define _LINUX_HMM_H_

/*
 * HMM/userptr is not supported by the NetBSD DRM compatibility layer.
 * Discard the argument so callers don't require a definition of hmm_range.
 */
#define	hmm_pfn_to_page(pfn)	NULL

#endif /* _LINUX_HMM_H_ */

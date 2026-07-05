/*	$NetBSD$	*/

#ifndef _LINUX_NODEMASK_H_
#define _LINUX_NODEMASK_H_

/* NetBSD does not expose Linux-style NUMA node masks to DRM. */
#define	num_possible_nodes()	1
#define	num_online_nodes()	1

#endif	/* _LINUX_NODEMASK_H_ */

/*	$NetBSD$	*/

/*
 * NetBSD has no Linux apple-gmux device interface.  Keep the associated
 * hybrid-graphics quirks disabled.
 */

#ifndef _LINUX_APPLE_GMUX_H_
#define _LINUX_APPLE_GMUX_H_

#include <linux/types.h>

static inline bool
apple_gmux_present(void)
{

	return false;
}

static inline bool
apple_gmux_detect(void *pnp_dev, void *type_ret)
{

	return false;
}

#endif /* _LINUX_APPLE_GMUX_H_ */

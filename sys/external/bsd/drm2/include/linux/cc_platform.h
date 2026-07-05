/*	$NetBSD$	*/

#ifndef _LINUX_CC_PLATFORM_H_
#define _LINUX_CC_PLATFORM_H_

#include <linux/types.h>

enum cc_attr {
	CC_ATTR_MEM_ENCRYPT,
	CC_ATTR_HOST_MEM_ENCRYPT,
	CC_ATTR_GUEST_MEM_ENCRYPT,
	CC_ATTR_GUEST_STATE_ENCRYPT,
	CC_ATTR_GUEST_UNROLL_STRING_IO,
	CC_ATTR_GUEST_SEV_SNP,
	CC_ATTR_GUEST_SNP_SECURE_TSC,
	CC_ATTR_HOST_SEV_SNP,
	CC_ATTR_SNP_SECURE_AVIC,
};

static inline bool
cc_platform_has(enum cc_attr attr)
{
	return false;
}

#endif

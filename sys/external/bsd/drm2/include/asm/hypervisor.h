/* Public domain. */

#ifndef _ASM_HYPERVISOR_H_
#define _ASM_HYPERVISOR_H_

#include <sys/stdbool.h>

#if defined(__i386__) || defined(__amd64__)

#define	X86_HYPER_NATIVE	1
#define	X86_HYPER_MS_HYPERV	2

static inline bool
hypervisor_is_type(int type)
{

	return type == X86_HYPER_NATIVE;
}

#endif

#endif	/* _ASM_HYPERVISOR_H_ */

#ifndef _ASM_FPU_API_H_
#define _ASM_FPU_API_H_

#include <x86/fpu.h>

#define	kernel_fpu_begin()	fpu_kern_enter()
#define	kernel_fpu_end()	fpu_kern_leave()

#endif

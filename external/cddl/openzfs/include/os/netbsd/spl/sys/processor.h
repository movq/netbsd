/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_PROCESSOR_H_
#define	_NETBSD_SPL_PROCESSOR_H_

#include_next <sys/processor.h>
#include <sys/cpu.h>

#define	getcpuid()	cpu_index(curcpu())

#endif

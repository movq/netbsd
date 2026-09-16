/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_SIMD_H_
#define	_NETBSD_SPL_SIMD_H_

/*
 * Select the portable checksum and RAID-Z implementations for bring-up.
 * Enabling SIMD also requires preserving the kernel's FPU context.
 */
#define	kfpu_allowed()		0
#define	kfpu_initialize(thread)	((void)0)
#define	kfpu_begin()		((void)0)
#define	kfpu_end()		((void)0)
#define	kfpu_init()		0
#define	kfpu_fini()		((void)0)
#define	simd_stat_init()		((void)0)
#define	simd_stat_fini()		((void)0)

#endif

/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_ABD_IMPL_OS_H_
#define	_NETBSD_ABD_IMPL_OS_H_

#include <sys/systm.h>

#define	abd_enter_critical(flags)	kpreempt_disable()
#define	abd_exit_critical(flags)	kpreempt_enable()

#endif

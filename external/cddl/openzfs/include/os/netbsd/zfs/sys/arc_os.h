/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_ARC_OS_H_
#define	_NETBSD_ARC_OS_H_

#include <sys/sysctl.h>

int param_set_arc_free_target(SYSCTLFN_ARGS);
int param_set_arc_no_grow_shift(SYSCTLFN_ARGS);

#endif

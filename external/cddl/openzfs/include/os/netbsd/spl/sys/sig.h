/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_SIG_H_
#define	_NETBSD_SPL_SIG_H_

#include <sys/proc.h>
#include <sys/signalvar.h>

static inline int
issig(void)
{
	return (sigispending(curlwp, 0) != 0);
}

#endif

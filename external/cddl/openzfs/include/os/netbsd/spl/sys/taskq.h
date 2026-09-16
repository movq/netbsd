/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_TASKQ_H_
#define	_NETBSD_SPL_TASKQ_H_

#include_next <sys/taskq.h>
#include <sys/systm.h>

static inline void
taskq_init_ent(taskq_ent_t *ent)
{
	memset(ent, 0, sizeof (*ent));
}

#endif

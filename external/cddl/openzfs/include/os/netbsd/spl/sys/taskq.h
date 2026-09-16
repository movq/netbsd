/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_TASKQ_H_
#define	_NETBSD_SPL_TASKQ_H_

#include_next <sys/taskq.h>
#include <sys/systm.h>

#define	TASKQID_INVALID	((taskqid_t)0)

/* As on FreeBSD, conservatively drain the whole queue. */
static inline void
taskq_wait_outstanding(taskq_t *tq, taskqid_t id)
{
	taskq_wait(tq);
}

static inline void
taskq_init_ent(taskq_ent_t *ent)
{
	memset(ent, 0, sizeof (*ent));
}

#endif

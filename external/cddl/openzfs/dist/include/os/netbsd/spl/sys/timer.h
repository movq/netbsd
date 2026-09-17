/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_TIMER_H_
#define	_NETBSD_SPL_TIMER_H_

#include <sys/systm.h>
#include <sys/time.h>

#define	ddi_time_after(a, b)	((long)((b) - (a)) < 0)
#define	ddi_time_after64(a, b)	((int64_t)((b) - (a)) < 0)

static inline void
usleep_range(unsigned long usec, unsigned long max_usec __unused)
{
	struct timeval tv = {
		.tv_sec = usec / MICROSEC,
		.tv_usec = usec % MICROSEC,
	};

	/* As with other tick-based NetBSD sleeps, round up to a whole tick. */
	(void)kpause("zfsdelay", false, tvtohz(&tv), NULL);
}

#endif

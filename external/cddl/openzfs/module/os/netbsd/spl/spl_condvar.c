/* SPDX-License-Identifier: BSD-2-Clause */

#include <sys/param.h>
#include <sys/condvar.h>
#include <sys/debug.h>
#include <sys/time.h>

/* The implementation calls the native NetBSD entry points. */
#undef	cv_wait_sig
#undef	cv_timedwait
#undef	cv_timedwait_sig

/*
 * OpenZFS expects 1 for a wakeup, 0 for a signal, and -1 for a timeout.
 * NetBSD returns zero for a wakeup and an errno otherwise.
 */
static int
spl_cv_result(int error)
{
	switch (error) {
	case 0:
		return (1);
	case EWOULDBLOCK:
		return (-1);
	case EINTR:
	case ERESTART:
		return (0);
	default:
		panic("%s: unexpected wait error %d", __func__, error);
	}
}

int
spl_cv_wait_sig(kcondvar_t *cv, kmutex_t *mtx)
{
	return (spl_cv_result(cv_wait_sig(cv, mtx)));
}

int
spl_cv_timedwait(kcondvar_t *cv, kmutex_t *mtx, clock_t deadline,
    boolean_t interruptible)
{
	/*
	 * NetBSD clock_t is unsigned; a signed difference handles lbolt
	 * rollover for deadlines less than half the counter range away.
	 * A native timeout of zero means forever, so never pass it through.
	 */
	CTASSERT(sizeof (clock_t) == sizeof (int));
	int ticks = (int)(deadline - ddi_get_lbolt());

	if (ticks <= 0)
		return (-1);
	int error = interruptible ? cv_timedwait_sig(cv, mtx, ticks) :
	    cv_timedwait(cv, mtx, ticks);
	return (spl_cv_result(error));
}

int
spl_cv_timedwait_hires(kcondvar_t *cv, kmutex_t *mtx, hrtime_t time,
    hrtime_t resolution, int flags, boolean_t interruptible)
{
	struct timespec ts;
	struct bintime timeout, epsilon;

	VERIFY0(flags & ~(CALLOUT_FLAG_ABSOLUTE | CALLOUT_FLAG_ROUNDUP));
	VERIFY3S(resolution, >=, 0);
	if (time <= 0)
		return (-1);
	if (flags & CALLOUT_FLAG_ABSOLUTE)
		time -= gethrtime();
	if (time <= 0)
		return (-1);

	if ((flags & CALLOUT_FLAG_ROUNDUP) && resolution > 0) {
		hrtime_t remainder = time % resolution;
		if (remainder != 0) {
			hrtime_t extra = resolution - remainder;
			time = time > INT64_MAX - extra ?
			    INT64_MAX : time + extra;
		}
	}
	ts.tv_sec = time / NANOSEC;
	ts.tv_nsec = time % NANOSEC;
	timespec2bintime(&ts, &timeout);
	ts.tv_sec = resolution / NANOSEC;
	ts.tv_nsec = resolution % NANOSEC;
	timespec2bintime(&ts, &epsilon);

	/*
	 * The native API may cap a very large interval to INT_MAX ticks.
	 * Continue after that timeout while there is still time remaining.
	 */
	int error;
	do {
		error = interruptible ?
		    cv_timedwaitbt_sig(cv, mtx, &timeout, &epsilon) :
		    cv_timedwaitbt(cv, mtx, &timeout, &epsilon);
	} while (error == EWOULDBLOCK &&
	    (timeout.sec > 0 || (timeout.sec == 0 && timeout.frac != 0)));
	return (spl_cv_result(error));
}

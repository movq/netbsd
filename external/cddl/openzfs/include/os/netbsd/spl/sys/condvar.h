/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_CONDVAR_H_
#define	_NETBSD_SPL_CONDVAR_H_

#include_next <sys/condvar.h>
#include <sys/types.h>
#include <sys/mutex.h>

typedef enum { CV_DEFAULT, CV_DRIVER } kcv_type_t;
typedef enum { TR_CLOCK_TICK } time_res_t;

#define	CALLOUT_FLAG_ROUNDUP	0x1
#define	CALLOUT_FLAG_ABSOLUTE	0x2

int spl_cv_wait_sig(kcondvar_t *, kmutex_t *);
int spl_cv_timedwait(kcondvar_t *, kmutex_t *, clock_t, boolean_t);
int spl_cv_timedwait_hires(kcondvar_t *, kmutex_t *, hrtime_t, hrtime_t,
    int, boolean_t);

#define	cv_init(cv, name, type, arg)	cv_init((cv), #cv)
#define	cv_wait_sig(cv, mtx)	spl_cv_wait_sig((cv), (mtx))
#define	cv_timedwait(cv, mtx, deadline) \
	spl_cv_timedwait((cv), (mtx), (deadline), B_FALSE)
#define	cv_timedwait_sig(cv, mtx, deadline) \
	spl_cv_timedwait((cv), (mtx), (deadline), B_TRUE)
#define	cv_timedwait_hires(cv, mtx, tim, res, flags) \
	spl_cv_timedwait_hires((cv), (mtx), (tim), (res), (flags), B_FALSE)
#define	cv_timedwait_sig_hires(cv, mtx, tim, res, flags) \
	spl_cv_timedwait_hires((cv), (mtx), (tim), (res), (flags), B_TRUE)
#define	cv_reltimedwait(cv, mtx, delta, res) \
	cv_timedwait((cv), (mtx), ddi_get_lbolt() + (delta))

#define	cv_wait_io		cv_wait
#define	cv_wait_io_sig		cv_wait_sig
#define	cv_wait_idle		cv_wait
#define	cv_timedwait_io		cv_timedwait
#define	cv_timedwait_sig_io	cv_timedwait_sig
#define	cv_timedwait_idle	cv_timedwait
#define	cv_timedwait_io_hires	cv_timedwait_hires
#define	cv_timedwait_idle_hires	cv_timedwait_hires

#endif

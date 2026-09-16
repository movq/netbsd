/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_ZFS_CONTEXT_OS_H_
#define	_NETBSD_ZFS_CONTEXT_OS_H_

#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/cred.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/rwlock.h>
#include <sys/pathname.h>
#include <sys/policy.h>
#include <sys/refstr.h>
#include <sys/sig.h>
#include <sys/tsd.h>
#include <sys/vfs.h>

#define	CPU_SEQID	(curcpu()->ci_data.cpu_index)
#define	CPU_SEQID_UNSTABLE	CPU_SEQID
#define	fm_panic	panic
#define	MSEC_TO_TICK(ms)	howmany((hrtime_t)(ms) * hz, MILLISEC)

/* NetBSD allocation does not invoke Linux-style filesystem shrinkers. */
typedef int fstrans_cookie_t;
#define	spl_fstrans_mark()	(0)
#define	spl_fstrans_unmark(cookie)	((void)(cookie))

#endif

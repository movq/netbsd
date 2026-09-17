/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_PROC_H_
#define	_NETBSD_SPL_PROC_H_

#include_next <sys/proc.h>

#define	getcomm()	(curproc->p_comm)
#define	getpid()	(curproc->p_pid)
#define	defclsyspri	minclsyspri
#define	wtqclsyspri	minclsyspri

static inline boolean_t
zfs_proc_is_caller(proc_t *proc)
{
	return (proc == curproc);
}

#define	thread_create_named(name, stk, stksz, func, arg, len, pp, state, pri) \
	solaris__thread_create((stk), (stksz), (func), (arg), (len), (pp), \
	    (state), (pri), (name))

#endif

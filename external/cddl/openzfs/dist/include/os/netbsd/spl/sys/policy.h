/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_POLICY_H_
#define	_NETBSD_SPL_POLICY_H_

#include_next <sys/policy.h>

/* The shared Solaris policy implementation takes a native vnode. */
#define	secpolicy_vnode_setid_retain(zp, cr, root)	\
	secpolicy_vnode_setid_retain(ZTOV(zp), cr, root)

#endif

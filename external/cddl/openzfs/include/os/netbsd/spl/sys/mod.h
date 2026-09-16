/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_MOD_H_
#define	_NETBSD_SPL_MOD_H_

/*
 * As in the old NetBSD integration, common-code tunables retain their
 * compiled-in defaults. Registration with sysctl belongs to the OS layer.
 */
#define	ZMOD_RW	0
#define	ZMOD_RD	0
#define	ZFS_MODULE_PARAM(scope, prefix, name, type, perm, desc)

#endif

/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_CRED_H_
#define	_NETBSD_SPL_CRED_H_

#include_next <sys/cred.h>

/* NetBSD has no per-mount uid/gid namespace. */
#define	KUID_TO_SUID(id)	(id)
#define	KGID_TO_SGID(id)	(id)
#define	SUID_TO_KUID(id)	(id)
#define	SGID_TO_KGID(id)	(id)

/* Preserve the old port's fallback identities for unmapped FUIDs. */
#define	UID_NOBODY	32767
#define	GID_NOBODY	39

#endif

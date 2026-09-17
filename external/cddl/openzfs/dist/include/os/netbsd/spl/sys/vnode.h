/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _NETBSD_SPL_VNODE_H_
#define	_NETBSD_SPL_VNODE_H_

/*
 * Bypass both osnet vnode headers: OpenZFS owns xvattr and vsecattr.
 * The module makefile adds the parent of the native sys tree.
 */
#include <sys/sys/vnode.h>
#include <sys/namei.h>
#include <sys/cred.h>
#include <sys/fcntl.h>

typedef struct vnode vnode_t;
typedef struct vattr vattr_t;
typedef enum vtype vtype_t;
typedef int (**vnodeops_t)(void *);
#define	VATTR_NULL(vap)	vattr_null(vap)
#define	ASSERT_VOP_LOCKED(vp, where)	KASSERT(VOP_ISLOCKED(vp) != 0)
#define	ASSERT_VOP_ELOCKED(vp, where)	\
	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE)

enum symfollow { NO_FOLLOW = NOFOLLOW };
enum rm { RMFILE, RMDIRECTORY };
enum create { CRCREAT, CRMKNOD, CRMKDIR };

/* Preserve the old NetBSD mapping of Solaris attributes. */
#define	va_mask		va_spare
#define	va_nodeid	va_fileid
#define	va_nblocks	va_bytes
#define	va_blksize	va_blocksize
#define	va_seq		va_gen

#define	AT_TYPE		0x00001
#define	AT_MODE		0x00002
#define	AT_UID		0x00004
#define	AT_GID		0x00008
#define	AT_FSID		0x00010
#define	AT_NODEID	0x00020
#define	AT_NLINK	0x00040
#define	AT_SIZE		0x00080
#define	AT_ATIME	0x00100
#define	AT_MTIME	0x00200
#define	AT_CTIME	0x00400
#define	AT_RDEV		0x00800
#define	AT_BLKSIZE	0x01000
#define	AT_NBLOCKS	0x02000
#define	AT_SEQ		0x08000
#define	AT_XVATTR	0x10000
#define	AT_ALL		(AT_TYPE | AT_MODE | AT_UID | AT_GID | AT_FSID | \
	AT_NODEID | AT_NLINK | AT_SIZE | AT_ATIME | AT_MTIME | AT_CTIME | \
	AT_RDEV | AT_BLKSIZE | AT_NBLOCKS | AT_SEQ)
#define	AT_TIMES	(AT_ATIME | AT_MTIME | AT_CTIME)
#define	AT_NOSET	(AT_NLINK | AT_RDEV | AT_FSID | AT_NODEID | \
	AT_BLKSIZE | AT_NBLOCKS | AT_SEQ)
#define	ATTR_UID	AT_UID
#define	ATTR_GID	AT_GID
#define	ATTR_MODE	AT_MODE
#define	ATTR_SIZE	AT_SIZE
#define	ATTR_XVATTR	AT_XVATTR
#define	ATTR_ATIME	AT_ATIME
#define	ATTR_MTIME	AT_MTIME
#define	ATTR_CTIME	AT_CTIME

#include <sys/xvattr.h>

static inline void
vattr_init_mask(vattr_t *vap)
{
	vap->va_mask = 0;
	if (vap->va_type != VNON)
		vap->va_mask |= AT_TYPE;
	if (vap->va_uid != (uid_t)VNOVAL)
		vap->va_mask |= AT_UID;
	if (vap->va_gid != (gid_t)VNOVAL)
		vap->va_mask |= AT_GID;
	if (vap->va_size != (u_quad_t)VNOVAL)
		vap->va_mask |= AT_SIZE;
	if (vap->va_atime.tv_sec != VNOVAL)
		vap->va_mask |= AT_ATIME;
	if (vap->va_mtime.tv_sec != VNOVAL)
		vap->va_mask |= AT_MTIME;
	if (vap->va_mode != (mode_t)VNOVAL)
		vap->va_mask |= AT_MODE;
	if (vap->va_flags != VNOVAL)
		vap->va_mask |= AT_XVATTR;
}

#define	MODEMASK	07777
#define	PERMMASK	00777
#define	IS_DEVVP(vp)	((vp)->v_type == VCHR || (vp)->v_type == VBLK || \
	(vp)->v_type == VFIFO)
#define	IS_XATTRDIR(vp)	(0)
#define	v_object	v_uobj
#define	v_lock		v_interlock
#define	VN_HOLD(vp)	vref(vp)
#define	VN_RELE(vp)	vrele(vp)
#define	VN_URELE(vp)	vput(vp)
#define	__DECONST(type, ptr)	((type)__UNCONST(ptr))
/* Solaris file events are not provided by NetBSD, as in osnet. */
#define	vnevent_create(vp, ct)	((void)0)
#define	vnevent_link(vp, ct)	((void)0)
#define	vnevent_remove(vp, dvp, name, ct)	((void)0)
#define	vnevent_rmdir(vp, dvp, name, ct)	((void)0)
#define	vnevent_rename_src(vp, dvp, name, ct)	((void)0)
#define	vnevent_rename_dest(vp, dvp, name, ct)	((void)0)
#define	vnevent_rename_dest_dir(vp, ct)	((void)0)
#define	VN_RELE_ASYNC(vp, tq)	vrele_async(vp)
#define	vn_has_cached_data(vp)	(((vp)->v_iflag & VI_PAGES) != 0)
#define	vn_ismntpt(vp)	((vp)->v_type == VDIR && (vp)->v_mountedhere != NULL)
#define	vn_mountedvfs(vp)	((vp)->v_mountedhere)
#define	vn_vfswlock(vp)	(0)
#define	vn_vfsunlock(vp)	((void)0)
#define	vn_matchops(vp, ops)	((vp)->v_op == &(ops))

typedef struct caller_context {
	pid_t		cc_pid;
	int		cc_sysid;
	u_longlong_t	cc_caller_id;
	ulong_t		cc_flags;
} caller_context_t;

#define	LOOKUP_DIR	0x01
#define	LOOKUP_XATTR	0x02
#define	CREATE_XATTR_DIR	0x04
#define	LOOKUP_HAVE_SYSATTR_DIR	0x08
#define	V_RDDIR_ENTFLAGS	0x01
#define	V_RDDIR_ACCFILTER	0x02
#define	ATTR_UTIME	0x01
#define	ATTR_EXEC	0x02
#define	ATTR_COMM	0x04
#define	ATTR_HINT	0x08
#define	ATTR_REAL	0x10
#define	ATTR_NOACLCHECK	0x20
#define	ATTR_TRIGGER	0x40

#define	FCREAT		O_CREAT
#define	FTRUNC		O_TRUNC
#define	FOFFMAX		0
#define	EXCL		0
#define	FIGNORECASE	0
#define	RLIM64_INFINITY	0

#include <sys/vfs.h>

#endif

/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Native helpers for the common vnode operations.
 */
#include <sys/zfs_context.h>
#include <sys/zfs_znode.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>

void
zfs_flush_cached_data(vnode_t *vp, boolean_t sync)
{
	int flags = PGO_ALLPAGES | PGO_CLEANIT;

	if (sync)
		flags |= PGO_SYNCIO;
	/* VOP_PUTPAGES consumes the VM object lock. */
	rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
	(void) VOP_PUTPAGES(vp, 0, 0, flags);
}

int
zfs_rlimit_fsize(uint64_t size)
{
	struct proc *p = curproc;

	if (size <= p->p_rlimit[RLIMIT_FSIZE].rlim_cur)
		return (0);

	mutex_enter(&proc_lock);
	psignal(p, SIGXFSZ);
	mutex_exit(&proc_lock);
	return (SET_ERROR(EFBIG));
}

int
zfs_rlimit_fsize_uio(vnode_t *vp, zfs_uio_t *uio)
{
	uint64_t offset = zfs_uio_offset(uio);
	uint64_t resid = zfs_uio_resid(uio);

	if (vp->v_type != VREG || zfs_uio_rw(uio) != UIO_WRITE)
		return (0);
	/*
	 * The common writer has resolved O_APPEND before calling us.
	 * Match NetBSD's whole-request limit check, avoiding signed overflow.
	 */
	if (resid > UINT64_MAX - offset)
		return (SET_ERROR(EFBIG));
	return (zfs_rlimit_fsize(offset + resid));
}

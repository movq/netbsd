/* SPDX-License-Identifier: BSD-2-Clause AND CDDL-1.0 */
/*
 * Native UVM operations, adapted from NetBSD osnet.
 * CDDL portions:
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2015 by Delphix. All rights reserved.
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

#include <sys/zfs_vnops.h>
#include <sys/zfs_vcache.h>
#include <sys/zfs_quota.h>
#include <sys/dmu_objset.h>
#include <sys/fstrans.h>
#include <sys/tsd.h>
#include <miscfs/genfs/genfs.h>
#include <uvm/uvm.h>

static uint_t zfs_putpage_key;

void
zfs_vm_init(void)
{
	tsd_create(&zfs_putpage_key, NULL);
}

void
zfs_vm_fini(void)
{
	tsd_destroy(&zfs_putpage_key);
}

caddr_t
zfs_map_page(page_t *pp, enum seg_rw rw)
{
	vaddr_t va;
	int flags;

	flags = UVMPAGER_MAPIN_WAITOK |
		(rw == S_READ ? UVMPAGER_MAPIN_WRITE : UVMPAGER_MAPIN_READ);
	va = uvm_pagermapin(&pp, 1, flags);
	return (caddr_t)va;
}

void
zfs_unmap_page(page_t *pp, caddr_t addr)
{

	uvm_pagermapout((vaddr_t)addr, 1);
}

int
mappedread(znode_t *zp, int nbytes, zfs_uio_t *uio)
{
	vnode_t *vp = ZTOV(zp);
	struct uvm_object *uobj = &vp->v_uobj;
	krwlock_t *rw = uobj->vmobjlock;
	int64_t start;
	caddr_t va;
	size_t len = nbytes;
	int off;
	int error = 0;
	int npages, found;
	void *buf = NULL;

	start = zfs_uio_offset(uio);
	off = start & PAGEOFFSET;

	for (start &= PAGEMASK; len > 0; start += PAGESIZE) {
		page_t *pp;
		uint64_t bytes = MIN(PAGESIZE - off, len);
retry:
		pp = NULL;
		npages = 1;
		rw_enter(rw, RW_WRITER);
		found = uvn_findpages(uobj, start, &npages, &pp, NULL,
		    UFP_NOALLOC);
		rw_exit(rw);

		if (found) {
			if (buf != NULL) {
				va = zfs_map_page(pp, S_READ);
				memcpy(buf, va + off, bytes);
				zfs_unmap_page(pp, va);
			}
			rw_enter(rw, RW_WRITER);
			uvm_page_unbusy(&pp, 1);
			rw_exit(rw);
			if (buf == NULL) {
				buf = kmem_alloc(PAGESIZE, KM_SLEEP);
				goto retry;
			}
			error = zfs_uiomove(buf, bytes, UIO_READ, uio);
		} else {
			error = dmu_read_uio_dbuf(sa_get_db(zp->z_sa_hdl),
			    uio, bytes, 0);
		}

		len -= bytes;
		off = 0;
		if (error)
			break;
	}
	if (buf != NULL) {
		kmem_free(buf, PAGESIZE);
	}
	return (error);
}

void
update_pages(znode_t *zp, int64_t start, int len, objset_t *os)
{
	vnode_t *vp = ZTOV(zp);
	uint64_t oid = zp->z_id;
	struct uvm_object *uobj = &vp->v_uobj;
	krwlock_t *rw = uobj->vmobjlock;
	caddr_t va;
	int off, status;

	ASSERT(vp->v_mount != NULL);

	rw_enter(rw, RW_WRITER);

	off = start & PAGEOFFSET;
	for (start &= PAGEMASK; len > 0; start += PAGESIZE) {
		page_t *pp;
		int nbytes = MIN(PAGESIZE - off, len);
		int npages, found;

		pp = NULL;
		npages = 1;
		found = uvn_findpages(uobj, start, &npages, &pp, NULL,
		    UFP_NOALLOC);
		if (found) {
			if (nbytes == PAGESIZE) {
				/*
				 * We're about to zap the page's contents
				 * and don't care about any existing
				 * modifications.  We must keep track of
				 * any new modifications past this point.
				 * Clear the modified bit in the pmap, and
				 * if the page is marked dirty revert to
				 * tracking the modified bit.
				 */
				switch (uvm_pagegetdirty(pp)) {
				case UVM_PAGE_STATUS_DIRTY:
					/* Does pmap_clear_modify(). */
					uvm_pagemarkdirty(pp, UVM_PAGE_STATUS_UNKNOWN);
					break;
				case UVM_PAGE_STATUS_UNKNOWN:
					pmap_clear_modify(pp);
					break;
				case UVM_PAGE_STATUS_CLEAN:
					/* Nothing to do. */
					break;
				}
			}
			rw_exit(rw);

			va = zfs_map_page(pp, S_WRITE);
			(void) dmu_read(os, oid, start + off, nbytes,
			    va + off, DMU_READ_PREFETCH);
			zfs_unmap_page(pp, va);

			rw_enter(rw, RW_WRITER);
			uvm_page_unbusy(&pp, 1);
		}
		len -= nbytes;
		off = 0;
	}
	rw_exit(rw);
}

int
zfs_netbsd_getpages(void *v)
{
	struct vop_getpages_args /* {
		struct vnode *a_vp;
		voff_t a_offset;
		struct vm_page **a_m;
		int *a_count;
		int a_centeridx;
		vm_prot_t a_access_type;
		int a_advice;
		int a_flags;
	} */ * const ap = v;

	vnode_t *const vp = ap->a_vp;
	const int flags = ap->a_flags;
	const bool async = (flags & PGO_SYNCIO) == 0;
	const bool memwrite = (ap->a_access_type & VM_PROT_WRITE) != 0;

	struct uvm_object * const uobj = &vp->v_uobj;
	krwlock_t * const rw = uobj->vmobjlock;
	znode_t *zp = VTOZ(vp);
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	vfs_t *mp;
	struct vm_page *pg;
	caddr_t va;
	int npages = *ap->a_count, found, err = 0;

	if (flags & PGO_LOCKED) {
		uvn_findpages(uobj, ap->a_offset, &npages, ap->a_m, NULL,
		    UFP_NOWAIT | UFP_NOALLOC | UFP_NOBUSY |
		    (memwrite ? UFP_NORDONLY : 0));
		KASSERT(npages == *ap->a_count);
		if (memwrite) {
			KASSERT(rw_write_held(uobj->vmobjlock));
			for (int i = 0; i < npages; i++) {
				pg = ap->a_m[i];
				if (pg == NULL || pg == PGO_DONTCARE) {
					continue;
				}
				if (uvm_pagegetdirty(pg) ==
				    UVM_PAGE_STATUS_CLEAN) {
					uvm_pagemarkdirty(pg,
					    UVM_PAGE_STATUS_UNKNOWN);
				}
			}
		}
		return ap->a_m[ap->a_centeridx] == NULL ? EBUSY : 0;
	}
	rw_exit(rw);

	if (async) {
		return 0;
	}

	mp = vp->v_mount;
	fstrans_start(mp);
	if (vp->v_mount != mp) {
		fstrans_done(mp);
		return ENOENT;
	}
	err = zfs_enter_verify_zp(zfsvfs, zp, FTAG);
	if (err != 0) {
		fstrans_done(mp);
		return (err);
	}

	rw_enter(rw, RW_WRITER);
	if (ap->a_offset > round_page(vp->v_size) ||
	    (uint64_t)npages * PAGE_SIZE >
	    round_page(vp->v_size) - ap->a_offset) {
		rw_exit(rw);
		zfs_exit(zfsvfs, FTAG);
		fstrans_done(mp);
		return EINVAL;
	}
	uvn_findpages(uobj, ap->a_offset, &npages, ap->a_m, NULL, UFP_ALL);
	KASSERT(npages == *ap->a_count);

	for (int i = 0; i < npages; i++) {
		pg = ap->a_m[i];
		if (pg->flags & PG_FAKE) {
			voff_t offset = pg->offset;
			KASSERT(pg->offset == ap->a_offset + (i << PAGE_SHIFT));
			rw_exit(rw);

			va = zfs_map_page(pg, S_WRITE);
			err = dmu_read(zfsvfs->z_os, zp->z_id, offset,
			    PAGE_SIZE, va, DMU_READ_PREFETCH);
			zfs_unmap_page(pg, va);

			if (err != 0) {
				uvm_aio_aiodone_pages(ap->a_m, npages, false, err);
				memset(ap->a_m, 0, sizeof(ap->a_m[0]) *
				    npages);
				goto out;
			}
			rw_enter(rw, RW_WRITER);
			pg->flags &= ~(PG_FAKE);
		}

		if (memwrite && uvm_pagegetdirty(pg) == UVM_PAGE_STATUS_CLEAN) {
			/* For write faults, start dirtiness tracking. */
			uvm_pagemarkdirty(pg, UVM_PAGE_STATUS_UNKNOWN);
		}
	}
	rw_exit(rw);

out:
	zfs_exit(zfsvfs, FTAG);
	fstrans_done(mp);

	return (err);
}


void
zfs_netbsd_update_mctime(vnode_t *vp)
{
	znode_t		*zp = VTOZ(vp);
	zfsvfs_t	*zfsvfs = zp->z_zfsvfs;
	dmu_tx_t	*tx;
	sa_bulk_attr_t	bulk[2];
	uint64_t	mtime[2], ctime[2];
	int		count = 0, err;

	if (zfs_enter_verify_zp(zfsvfs, zp, FTAG) != 0)
		return;
	tx = dmu_tx_create(zfsvfs->z_os);
	dmu_tx_hold_sa(tx, zp->z_sa_hdl, B_FALSE);
	zfs_sa_upgrade_txholds(tx, zp);
	err = dmu_tx_assign(tx, DMU_TX_WAIT);
	if (err != 0) {
		dmu_tx_abort(tx);
		zfs_exit(zfsvfs, FTAG);
		return;
	}
	SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_MTIME(zfsvfs), NULL, &mtime, 16);
	SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_CTIME(zfsvfs), NULL, &ctime, 16);
	zfs_tstamp_update_setup(zp, CONTENT_MODIFIED, mtime, ctime);
	err = sa_bulk_update(zp->z_sa_hdl, bulk, count, tx);
	dmu_tx_commit(tx);
	if (err != 0) {
		printf("%s: sa_bulk_update failed with %d\n", __func__, err);
	}
	zfs_exit(zfsvfs, FTAG);
}

static int
zfs_putapage(vnode_t *vp, struct vm_page **pages, int npages, int flags)
{
	znode_t *zp = VTOZ(vp);
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	dmu_tx_t *tx;
	uint64_t off = pages[0]->offset, len = (uint64_t)npages * PAGE_SIZE;
	vaddr_t va;
	int error = 0;

	if (zp->z_sa_hdl == NULL)
		goto out;
	/* DMU allocation and transaction waits are unsafe in the pagedaemon. */
	if (uvm_lwp_is_pagedaemon(curlwp)) {
		error = ENOMEM;
		goto out;
	}
	ASSERT3U(off, <=, zp->z_size);
	ASSERT3U(len, <=, round_page(zp->z_size) - off);
	len = MIN(len, zp->z_size - off);
	if (zfs_id_overblockquota(zfsvfs, DMU_USERUSED_OBJECT, zp->z_uid) ||
	    zfs_id_overblockquota(zfsvfs, DMU_GROUPUSED_OBJECT, zp->z_gid) ||
	    (zp->z_projid != ZFS_DEFAULT_PROJID &&
	    zfs_id_overblockquota(zfsvfs, DMU_PROJECTUSED_OBJECT, zp->z_projid))) {
		error = EDQUOT;
		goto out;
	}
	tx = dmu_tx_create(zfsvfs->z_os);
	dmu_tx_hold_write(tx, zp->z_id, off, len);
	dmu_tx_hold_sa(tx, zp->z_sa_hdl, B_FALSE);
	zfs_sa_upgrade_txholds(tx, zp);
	error = dmu_tx_assign(tx, DMU_TX_WAIT);
	if (error != 0) {
		dmu_tx_abort(tx);
		goto out;
	}
	/*
	 * genfs supplies consecutive busy pages. Copy their mapped bytes into
	 * the ARC synchronously, as in osnet's dmu_write_pages adapter.
	 */
	va = uvm_pagermapin(pages, npages,
	    UVMPAGER_MAPIN_WAITOK | UVMPAGER_MAPIN_WRITE);
	struct iovec iov = { .iov_base = (void *)va, .iov_len = len };
	struct uio native = {
		.uio_iov = &iov, .uio_iovcnt = 1, .uio_offset = off,
		.uio_resid = len, .uio_rw = UIO_WRITE,
		.uio_vmspace = vmspace_kernel()
	};
	zfs_uio_t uio;
	zfs_uio_init(&uio, &native);
	error = dmu_write_uio_dbuf(sa_get_db(zp->z_sa_hdl), &uio, len, tx, 0);
	uvm_pagermapout(va, npages);
	if (error == 0) {
		uint64_t mtime[2], ctime[2];
		sa_bulk_attr_t bulk[3];
		int count = 0;

		SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_MTIME(zfsvfs), NULL,
		    mtime, sizeof (mtime));
		SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_CTIME(zfsvfs), NULL,
		    ctime, sizeof (ctime));
		SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_FLAGS(zfsvfs), NULL,
		    &zp->z_pflags, sizeof (zp->z_pflags));
		zfs_tstamp_update_setup(zp, CONTENT_MODIFIED, mtime, ctime);
		error = sa_bulk_update(zp->z_sa_hdl, bulk, count, tx);
		zfs_log_write(zfsvfs->z_log, tx, TX_WRITE, zp, off, len,
		    B_FALSE, B_FALSE, NULL, NULL);
		*(bool *)tsd_get(zfs_putpage_key) = true;
	}
	dmu_tx_commit(tx);
out:
	uvm_aio_aiodone_pages(pages, npages, true, error);
	return (error);
}

int
zfs_netbsd_putpages(void *v)
{
	struct vop_putpages_args *ap = v;
	vnode_t *vp = ap->a_vp;
	znode_t *zp = VTOZ(vp);
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	struct mount *mp = vp->v_mount;
	zfs_locked_range_t *lr;
	bool cleaned = false;
	void *previous;
	int error;

	if ((ap->a_flags & PGO_CLEANIT) == 0)
		return (genfs_putpages(v));
	rw_exit(vp->v_uobj.vmobjlock);
	if (uvm_lwp_is_pagedaemon(curlwp)) {
		error = fstrans_start_nowait(mp);
		if (error != 0)
			return (error);
	} else {
		fstrans_start(mp);
	}
	if (vp->v_mount != mp) {
		fstrans_done(mp);
		return (0);
	}
	/* Forced unmount must still be able to drain cached pages. */
	ZFS_TEARDOWN_ENTER_READ(zfsvfs, FTAG);
	uint64_t len = ap->a_offhi == 0 ? UINT64_MAX :
	    ap->a_offhi - ap->a_offlo;
	if (uvm_lwp_is_pagedaemon(curlwp)) {
		lr = zfs_rangelock_tryenter(&zp->z_rangelock, ap->a_offlo,
		    len, RL_WRITER);
		if (lr == NULL) {
			error = EBUSY;
			goto out;
		}
	} else {
		lr = zfs_rangelock_enter(&zp->z_rangelock, ap->a_offlo,
		    len, RL_WRITER);
	}
	previous = tsd_get(zfs_putpage_key);
	tsd_set(zfs_putpage_key, &cleaned);
	rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
	error = genfs_putpages(v);
	tsd_set(zfs_putpage_key, previous);
	zfs_rangelock_exit(lr);
	/* Avoid a ZIL callback into our own vnode during reclaim or truncate. */
	if (cleaned && (ap->a_flags & PGO_RECLAIM) == 0 &&
	    ((ap->a_flags & PGO_SYNCIO) ||
	    zfsvfs->z_os->os_sync == ZFS_SYNC_ALWAYS)) {
		int log_error = zil_commit(zfsvfs->z_log, zp->z_id);
		if (error == 0)
			error = log_error;
	}
out:
	zfs_exit(zfsvfs, FTAG);
	fstrans_done(mp);
	return (error);
}

static void
zfs_netbsd_gop_putrange(vnode_t *vp, off_t off, off_t *lo, off_t *hi)
{
	znode_t *zp = VTOZ(vp);
	uint64_t blksz = MAX(zp->z_blksz, PAGE_SIZE);

	*lo = trunc_page(P2ALIGN_TYPED(off, blksz, uint64_t));
	*hi = round_page(*lo + blksz);
}

const struct genfs_ops zfs_genfsops = {
	.gop_write = zfs_putapage,
	.gop_putrange = zfs_netbsd_gop_putrange,
};

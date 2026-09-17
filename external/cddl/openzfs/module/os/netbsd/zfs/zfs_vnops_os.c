// SPDX-License-Identifier: CDDL-1.0
/* NetBSD vnode operations, adapted from OpenZFS 2.4.4 and osnet. */
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or https://opensource.org/licenses/CDDL-1.0.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2015 by Delphix. All rights reserved.
 * Copyright (c) 2014 Integros [integros.com]
 * Copyright 2017 Nexenta Systems, Inc.
 * Copyright (c) 2025, Klara, Inc.
 */

/* Portions Copyright 2007 Jeremy Teo */
/* Portions Copyright 2010 Robert Milkowski */

#include <sys/zfs_context.h>
#include <sys/zfs_dir.h>
#include <sys/zfs_ioctl.h>
#include <sys/dmu_objset.h>
#include <sys/dsl_dataset.h>
#include <sys/zap.h>
#include <sys/zfs_ctldir.h>
#include <sys/zfs_fuid.h>
#include <sys/zfs_quota.h>
#include <sys/zfs_sa.h>
#include <sys/zfs_project.h>
#include <sys/zfs_vnops.h>
#include <sys/dmu_impl.h>
#include <sys/zfeature.h>

/* vcache_new handles native vnode allocation. */
#define	getnewvnode_reserve()	((void)0)
#define	getnewvnode_drop_reserve()	((void)0)
#define	VNASSERT(cond, vp, msg)	KASSERT(cond)
/* Native namecache entries are invalidated under the vnode lock. */
#define	vn_seqc_write_begin(vp)	((void)0)
#define	vn_seqc_write_end(vp)	((void)0)
#define	cache_vop_rmdir(dvp, vp)	cache_purge(vp)
#define	cache_vop_rename(sd, sv, td, tv, sc, tc) do {	\
	cache_purge(sd); cache_purge(td); cache_purge(sv);	\
	if ((tv) != NULL) cache_purge(tv);			\
} while (0)

/*
 * Programming rules.
 *
 * Each vnode op performs some logical unit of work.  To do this, the ZPL must
 * properly lock its in-core state, create a DMU transaction, do the work,
 * record this work in the intent log (ZIL), commit the DMU transaction,
 * and wait for the intent log to commit if it is a synchronous operation.
 * Moreover, the vnode ops must work in both normal and log replay context.
 * The ordering of events is important to avoid deadlocks and references
 * to freed memory.  The example below illustrates the following Big Rules:
 *
 *  (1)	A check must be made in each zfs thread for a mounted file system.
 *	This is done avoiding races using zfs_enter(zfsvfs).
 *	A zfs_exit(zfsvfs) is needed before all returns.  Any znodes
 *	must be checked with zfs_verify_zp(zp).  Both of these macros
 *	can return EIO from the calling function.
 *
 *  (2)	VN_RELE() should always be the last thing except for zil_commit()
 *	(if necessary) and zfs_exit(). This is for 3 reasons:
 *	First, if it's the last reference, the vnode/znode
 *	can be freed, so the zp may point to freed memory.  Second, the last
 *	reference will call zfs_zinactive(), which may induce a lot of work --
 *	pushing cached pages (which acquires range locks) and syncing out
 *	cached atime changes.  Third, zfs_zinactive() may require a new tx,
 *	which could deadlock the system if you were already holding one.
 *	If you must call VN_RELE() within a tx then use VN_RELE_ASYNC().
 *
 *  (3)	All range locks must be grabbed before calling dmu_tx_assign(),
 *	as they can span dmu_tx_assign() calls.
 *
 *  (4) If ZPL locks are held, pass DMU_TX_NOWAIT as the second argument to
 *      dmu_tx_assign().  This is critical because we don't want to block
 *      while holding locks.
 *
 *	If no ZPL locks are held (aside from zfs_enter()), use DMU_TX_WAIT.
 *	This reduces lock contention and CPU usage when we must wait (note
 *	that if throughput is constrained by the storage, nearly every
 *	transaction must wait).
 *
 *      Note, in particular, that if a lock is sometimes acquired before
 *      the tx assigns, and sometimes after (e.g. z_lock), then failing
 *      to use a non-blocking assign can deadlock the system.  The scenario:
 *
 *	Thread A has grabbed a lock before calling dmu_tx_assign().
 *	Thread B is in an already-assigned tx, and blocks for this lock.
 *	Thread A calls dmu_tx_assign(DMU_TX_WAIT) and blocks in
 *	txg_wait_open() forever, because the previous txg can't quiesce
 *	until B's tx commits.
 *
 *	If dmu_tx_assign() returns ERESTART and zfsvfs->z_assign is
 *	DMU_TX_NOWAIT, then drop all locks, call dmu_tx_wait(), and try
 *	again.  On subsequent calls to dmu_tx_assign(), pass
 *	DMU_TX_NOTHROTTLE in addition to DMU_TX_NOWAIT, to indicate that
 *	this operation has already called dmu_tx_wait().  This will ensure
 *	that we don't retry forever, waiting a short bit each time.
 *
 *  (5)	If the operation succeeded, generate the intent log entry for it
 *	before dropping locks.  This ensures that the ordering of events
 *	in the intent log matches the order in which they actually occurred.
 *	During ZIL replay the zfs_log_* functions will update the sequence
 *	number to indicate the zil transaction has replayed.
 *
 *  (6)	At the end of each vnode op, the DMU tx must always commit,
 *	regardless of whether there were any errors.
 *
 *  (7)	After dropping all locks, invoke zil_commit(zilog, foid)
 *	to ensure that synchronous semantics are provided when necessary.
 *
 * In general, this is how things should be ordered in each vnode op:
 *
 *	zfs_enter(zfsvfs);		// exit if unmounted
 * top:
 *	zfs_dirent_lookup(&dl, ...)	// lock directory entry (may VN_HOLD())
 *	rw_enter(...);			// grab any other locks you need
 *	tx = dmu_tx_create(...);	// get DMU tx
 *	dmu_tx_hold_*();		// hold each object you might modify
 *	error = dmu_tx_assign(tx,
 *	    (waited ? DMU_TX_NOTHROTTLE : 0) | DMU_TX_NOWAIT);
 *	if (error) {
 *		rw_exit(...);		// drop locks
 *		zfs_dirent_unlock(dl);	// unlock directory entry
 *		VN_RELE(...);		// release held vnodes
 *		if (error == ERESTART) {
 *			waited = B_TRUE;
 *			dmu_tx_wait(tx);
 *			dmu_tx_abort(tx);
 *			goto top;
 *		}
 *		dmu_tx_abort(tx);	// abort DMU tx
 *		zfs_exit(zfsvfs);	// finished in zfs
 *		return (error);		// really out of space
 *	}
 *	error = do_real_work();		// do whatever this VOP does
 *	if (error == 0)
 *		zfs_log_*(...);		// on success, make ZIL entry
 *	dmu_tx_commit(tx);		// commit DMU tx -- error or not
 *	rw_exit(...);			// drop locks
 *	zfs_dirent_unlock(dl);		// unlock directory entry
 *	VN_RELE(...);			// release held vnodes
 *	zil_commit(zilog, foid);	// synchronous when necessary
 *	zfs_exit(zfsvfs);		// finished in zfs
 *	return (error);			// done, report error
 */
static int
zfs_open(vnode_t **vpp, int flag, cred_t *cr)
{
	(void) cr;
	znode_t	*zp = VTOZ(*vpp);
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	int error;

	if ((error = zfs_enter_verify_zp(zfsvfs, zp, FTAG)) != 0)
		return (error);

	if ((flag & FWRITE) && (zp->z_pflags & ZFS_APPENDONLY) &&
	    ((flag & FAPPEND) == 0)) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EPERM));
	}

	/*
	 * Keep a count of the synchronous opens in the znode.  On first
	 * synchronous open we must convert all previous async transactions
	 * into sync to keep correct ordering.
	 * Skip it for snapshot, as it won't have any transactions.
	 */
	if (!zfsvfs->z_issnap && (flag & O_SYNC)) {
		if (atomic_inc_32_nv(&zp->z_sync_cnt) == 1)
			zil_async_to_sync(zfsvfs->z_log, zp->z_id);
	}

	zfs_exit(zfsvfs, FTAG);
	return (0);
}

static int
zfs_close(vnode_t *vp, int flag, int count, offset_t offset, cred_t *cr)
{
	(void) offset, (void) cr;
	znode_t	*zp = VTOZ(vp);
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	int error;

	if ((error = zfs_enter_verify_zp(zfsvfs, zp, FTAG)) != 0)
		return (error);

	/* Decrement the synchronous opens in the znode */
	if (!zfsvfs->z_issnap && (flag & O_SYNC) && (count == 1))
		atomic_dec_32(&zp->z_sync_cnt);

	zfs_exit(zfsvfs, FTAG);
	return (0);
}


int
zfs_write_simple(znode_t *zp, const void *data, size_t len,
    loff_t pos, size_t *presid)
{
	int error = 0;
	ssize_t resid;

	error = vn_rdwr(UIO_WRITE, ZTOV(zp), __DECONST(void *, data), len, pos,
	    UIO_SYSSPACE, IO_SYNC, kcred, &resid, curlwp);

	if (error) {
		return (SET_ERROR(error));
	} else if (presid == NULL) {
		if (resid != 0) {
			error = SET_ERROR(EIO);
		}
	} else {
		*presid = resid;
	}
	return (error);
}

void
zfs_zrele_async(znode_t *zp)
{
	vnode_t *vp = ZTOV(zp);
	objset_t *os = ITOZSB(vp)->z_os;

	VN_RELE_ASYNC(vp, dsl_pool_zrele_taskq(dmu_objset_pool(os)));
}


static inline bool
is_nametoolong(zfsvfs_t *zfsvfs, const char *name)
{
	size_t dlen = strlen(name);
	return (dlen > KERNEL_NAME_MAX);
}

/*
 * Attempt to create a new entry in a directory.  If the entry
 * already exists, truncate the file if permissible, else return
 * an error.  Return the vp of the created or trunc'd file.
 *
 *	IN:	dvp	- vnode of directory to put new file entry in.
 *		name	- name of new file entry.
 *		vap	- attributes of new file.
 *		excl	- flag indicating exclusive or non-exclusive mode.
 *		mode	- mode to open file with.
 *		cr	- credentials of caller.
 *		flag	- large file flag [UNUSED].
 *		ct	- caller context
 *		vsecp	- ACL to be set
 *		mnt_ns	- Unused on FreeBSD
 *
 *	OUT:	vpp	- vnode of created or trunc'd entry.
 *
 *	RETURN:	0 on success, error code on failure.
 *
 * Timestamps:
 *	dvp - ctime|mtime updated if new entry created
 *	 vp - ctime|mtime always, atime if new
 */
int
zfs_create(znode_t *dzp, const char *name, vattr_t *vap, int excl, int mode,
    znode_t **zpp, cred_t *cr, int flag, vsecattr_t *vsecp, zidmap_t *mnt_ns)
{
	(void) excl, (void) mode, (void) flag;
	znode_t		*zp;
	zfsvfs_t	*zfsvfs = dzp->z_zfsvfs;
	zilog_t		*zilog;
	objset_t	*os;
	dmu_tx_t	*tx;
	int		error;
	uid_t		uid = crgetuid(cr);
	gid_t		gid = crgetgid(cr);
	uint64_t	projid = ZFS_DEFAULT_PROJID;
	zfs_acl_ids_t   acl_ids;
	boolean_t	fuid_dirtied;
	uint64_t	txtype;

	if (is_nametoolong(zfsvfs, name))
		return (SET_ERROR(ENAMETOOLONG));

	/*
	 * If we have an ephemeral id, ACL, or XVATTR then
	 * make sure file system is at proper version
	 */
	if (zfsvfs->z_use_fuids == B_FALSE &&
	    (vsecp || (vap->va_mask & AT_XVATTR) ||
	    IS_EPHEMERAL(uid) || IS_EPHEMERAL(gid)))
		return (SET_ERROR(EINVAL));

	if ((error = zfs_enter_verify_zp(zfsvfs, dzp, FTAG)) != 0)
		return (error);
	os = zfsvfs->z_os;
	zilog = zfsvfs->z_log;

	if (zfsvfs->z_utf8 && u8_validate(name, strlen(name),
	    NULL, U8_VALIDATE_ENTIRE, &error) < 0) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EILSEQ));
	}

	if (vap->va_mask & AT_XVATTR) {
		if ((error = secpolicy_xvattr(ZTOV(dzp), (xvattr_t *)vap,
		    crgetuid(cr), cr, vap->va_type)) != 0) {
			zfs_exit(zfsvfs, FTAG);
			return (error);
		}
	}

	*zpp = NULL;

	if ((vap->va_mode & S_ISVTX) && secpolicy_vnode_stky_modify(cr))
		vap->va_mode &= ~S_ISVTX;

	error = zfs_dirent_lookup(dzp, name, &zp, ZNEW);
	if (error) {
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}
	ASSERT0P(zp);

	/*
	 * Create a new file object and update the directory
	 * to reference it.
	 */
	if ((error = zfs_zaccess(dzp, ACE_ADD_FILE, 0, B_FALSE, cr, mnt_ns))) {
		goto out;
	}

	/*
	 * We only support the creation of regular files in
	 * extended attribute directories.
	 */

	if ((dzp->z_pflags & ZFS_XATTR) &&
	    (vap->va_type != VREG)) {
		error = SET_ERROR(EINVAL);
		goto out;
	}

	if ((error = zfs_acl_ids_create(dzp, 0, vap,
	    cr, vsecp, &acl_ids, NULL)) != 0)
		goto out;

	if (S_ISREG(vap->va_mode) || S_ISDIR(vap->va_mode))
		projid = zfs_inherit_projid(dzp);
	if (zfs_acl_ids_overquota(zfsvfs, &acl_ids, projid)) {
		zfs_acl_ids_free(&acl_ids);
		error = SET_ERROR(EDQUOT);
		goto out;
	}

	getnewvnode_reserve();

	tx = dmu_tx_create(os);

	dmu_tx_hold_sa_create(tx, acl_ids.z_aclp->z_acl_bytes +
	    ZFS_SA_BASE_ATTR_SIZE);

	fuid_dirtied = zfsvfs->z_fuid_dirty;
	if (fuid_dirtied)
		zfs_fuid_txhold(zfsvfs, tx);
	dmu_tx_hold_zap(tx, dzp->z_id, TRUE, name);
	dmu_tx_hold_sa(tx, dzp->z_sa_hdl, B_FALSE);
	if (!zfsvfs->z_use_sa &&
	    acl_ids.z_aclp->z_acl_bytes > ZFS_ACE_SPACE) {
		dmu_tx_hold_write(tx, DMU_NEW_OBJECT,
		    0, acl_ids.z_aclp->z_acl_bytes);
	}
	error = dmu_tx_assign(tx, DMU_TX_WAIT);
	if (error) {
		zfs_acl_ids_free(&acl_ids);
		dmu_tx_abort(tx);
		getnewvnode_drop_reserve();
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}
	zfs_mknode(dzp, vap, tx, cr, 0, &zp, &acl_ids);

	error = zfs_link_create(dzp, name, zp, tx, ZNEW);
	if (error != 0) {
		/*
		 * Since, we failed to add the directory entry for it,
		 * delete the newly created dnode.
		 */
		zfs_znode_delete(zp, tx);
		VOP_UNLOCK(ZTOV(zp));
		zrele(zp);
		zfs_acl_ids_free(&acl_ids);
		dmu_tx_commit(tx);
		getnewvnode_drop_reserve();
		goto out;
	}

	if (fuid_dirtied)
		zfs_fuid_sync(zfsvfs, tx);

	txtype = zfs_log_create_txtype(Z_FILE, vsecp, vap);
	zfs_log_create(zilog, tx, txtype, dzp, zp, name,
	    vsecp, acl_ids.z_fuidp, vap);
	zfs_acl_ids_free(&acl_ids);
	dmu_tx_commit(tx);

	getnewvnode_drop_reserve();

out:
	VNASSERT(ZTOV(dzp)->v_holdcnt > 0 && ZTOV(dzp)->v_usecount > 0,
	    ZTOV(dzp), ("%s: wrong ref counts", __func__));
	if (error == 0) {
		*zpp = zp;
	}

	if (error == 0 && zfsvfs->z_os->os_sync == ZFS_SYNC_ALWAYS)
		error = zil_commit(zilog, 0);

	zfs_exit(zfsvfs, FTAG);
	return (error);
}

/*
 * Remove an entry from a directory.
 *
 *	IN:	dvp	- vnode of directory to remove entry from.
 *		name	- name of entry to remove.
 *		cr	- credentials of caller.
 *		ct	- caller context
 *		flags	- case flags
 *
 *	RETURN:	0 on success, error code on failure.
 *
 * Timestamps:
 *	dvp - ctime|mtime
 *	 vp - ctime (if nlink > 0)
 */
static int
zfs_remove_(vnode_t *dvp, vnode_t *vp, const char *name, cred_t *cr)
{
	znode_t		*dzp = VTOZ(dvp);
	znode_t		*zp;
	znode_t		*xzp;
	zfsvfs_t	*zfsvfs = dzp->z_zfsvfs;
	zilog_t		*zilog;
	uint64_t	xattr_obj;
	uint64_t	obj = 0;
	dmu_tx_t	*tx;
	boolean_t	unlinked;
	uint64_t	txtype;
	int		error;


	if ((error = zfs_enter_verify_zp(zfsvfs, dzp, FTAG)) != 0)
		return (error);
	zp = VTOZ(vp);
	if ((error = zfs_verify_zp(zp)) != 0) {
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}
	zilog = zfsvfs->z_log;

	xattr_obj = 0;
	xzp = NULL;

	if ((error = zfs_zaccess_delete(dzp, zp, cr, NULL))) {
		goto out;
	}

	/*
	 * Need to use rmdir for removing directories.
	 */
	if (vp->v_type == VDIR) {
		error = SET_ERROR(EPERM);
		goto out;
	}

	vnevent_remove(vp, dvp, name, ct);

	obj = zp->z_id;

	/* are there any extended attributes? */
	error = sa_lookup(zp->z_sa_hdl, SA_ZPL_XATTR(zfsvfs),
	    &xattr_obj, sizeof (xattr_obj));
	if (error == 0 && xattr_obj) {
		error = zfs_zget(zfsvfs, xattr_obj, &xzp);
		ASSERT0(error);
	}

	/*
	 * We may delete the znode now, or we may put it in the unlinked set;
	 * it depends on whether we're the last link, and on whether there are
	 * other holds on the vnode.  So we dmu_tx_hold() the right things to
	 * allow for either case.
	 */
	tx = dmu_tx_create(zfsvfs->z_os);
	dmu_tx_hold_zap(tx, dzp->z_id, FALSE, name);
	dmu_tx_hold_sa(tx, zp->z_sa_hdl, B_FALSE);
	zfs_sa_upgrade_txholds(tx, zp);
	zfs_sa_upgrade_txholds(tx, dzp);

	if (xzp) {
		dmu_tx_hold_sa(tx, zp->z_sa_hdl, B_TRUE);
		dmu_tx_hold_sa(tx, xzp->z_sa_hdl, B_FALSE);
	}

	/* charge as an update -- would be nice not to charge at all */
	dmu_tx_hold_zap(tx, zfsvfs->z_unlinkedobj, FALSE, NULL);

	/*
	 * Mark this transaction as typically resulting in a net free of space
	 */
	dmu_tx_mark_netfree(tx);

	error = dmu_tx_assign(tx, DMU_TX_WAIT);
	if (error) {
		dmu_tx_abort(tx);
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	/*
	 * Remove the directory entry.
	 */
	error = zfs_link_destroy(dzp, name, zp, tx, ZEXISTS, &unlinked);

	if (error) {
		dmu_tx_commit(tx);
		goto out;
	}

	if (unlinked) {
		zfs_unlinked_add(zp, tx);
	}
	/* XXX check changes to linux vnops */
	txtype = TX_REMOVE;
	zfs_log_remove(zilog, tx, txtype, dzp, name, obj, unlinked);

	dmu_tx_commit(tx);
out:

	if (xzp)
		vrele(ZTOV(xzp));

	if (error == 0 && zfsvfs->z_os->os_sync == ZFS_SYNC_ALWAYS)
		error = zil_commit(zilog, 0);

	zfs_exit(zfsvfs, FTAG);
	return (error);
}


static int
zfs_lookup_internal(znode_t *dzp, const char *name, vnode_t **vpp,
    struct componentname *cnp, int nameiop)
{
	znode_t *zp;
	int error;

	*vpp = NULL;
	error = zfs_dirent_lookup(dzp, name, &zp, ZEXISTS);
	if (error != 0)
		return (error);
	*vpp = ZTOV(zp);
	error = vn_lock(*vpp, LK_EXCLUSIVE);
	if (error != 0) {
		vrele(*vpp);
		*vpp = NULL;
	}
	return (error);
}

int
zfs_remove(znode_t *dzp, const char *name, cred_t *cr, int flags)
{
	vnode_t *vp;
	int error;
	struct componentname cn;

	if ((error = zfs_lookup_internal(dzp, name, &vp, &cn, DELETE)))
		return (error);

	error = zfs_remove_(ZTOV(dzp), vp, name, cr);
	vput(vp);
	return (error);
}
/*
 * Create a new directory and insert it into dvp using the name
 * provided.  Return a pointer to the inserted directory.
 *
 *	IN:	dvp	- vnode of directory to add subdir to.
 *		dirname	- name of new directory.
 *		vap	- attributes of new directory.
 *		cr	- credentials of caller.
 *		ct	- caller context
 *		flags	- case flags
 *		vsecp	- ACL to be set
 *		mnt_ns	- Unused on FreeBSD
 *
 *	OUT:	vpp	- vnode of created directory.
 *
 *	RETURN:	0 on success, error code on failure.
 *
 * Timestamps:
 *	dvp - ctime|mtime updated
 *	 vp - ctime|mtime|atime updated
 */
int
zfs_mkdir(znode_t *dzp, const char *dirname, vattr_t *vap, znode_t **zpp,
    cred_t *cr, int flags, vsecattr_t *vsecp, zidmap_t *mnt_ns)
{
	(void) flags, (void) vsecp;
	znode_t		*zp;
	zfsvfs_t	*zfsvfs = dzp->z_zfsvfs;
	zilog_t		*zilog;
	uint64_t	txtype;
	dmu_tx_t	*tx;
	int		error;
	uid_t		uid = crgetuid(cr);
	gid_t		gid = crgetgid(cr);
	zfs_acl_ids_t   acl_ids;
	boolean_t	fuid_dirtied;

	ASSERT3U(vap->va_type, ==, VDIR);

	if (is_nametoolong(zfsvfs, dirname))
		return (SET_ERROR(ENAMETOOLONG));

	/*
	 * If we have an ephemeral id, ACL, or XVATTR then
	 * make sure file system is at proper version
	 */
	if (zfsvfs->z_use_fuids == B_FALSE &&
	    ((vap->va_mask & AT_XVATTR) ||
	    IS_EPHEMERAL(uid) || IS_EPHEMERAL(gid)))
		return (SET_ERROR(EINVAL));

	if ((error = zfs_enter_verify_zp(zfsvfs, dzp, FTAG)) != 0)
		return (error);
	zilog = zfsvfs->z_log;

	if (dzp->z_pflags & ZFS_XATTR) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EINVAL));
	}

	if (zfsvfs->z_utf8 && u8_validate(dirname,
	    strlen(dirname), NULL, U8_VALIDATE_ENTIRE, &error) < 0) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EILSEQ));
	}

	if (vap->va_mask & AT_XVATTR) {
		if ((error = secpolicy_xvattr(ZTOV(dzp), (xvattr_t *)vap,
		    crgetuid(cr), cr, vap->va_type)) != 0) {
			zfs_exit(zfsvfs, FTAG);
			return (error);
		}
	}

	if ((error = zfs_acl_ids_create(dzp, 0, vap, cr,
	    NULL, &acl_ids, NULL)) != 0) {
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	/*
	 * First make sure the new directory doesn't exist.
	 *
	 * Existence is checked first to make sure we don't return
	 * EACCES instead of EEXIST which can cause some applications
	 * to fail.
	 */
	*zpp = NULL;

	if ((error = zfs_dirent_lookup(dzp, dirname, &zp, ZNEW))) {
		zfs_acl_ids_free(&acl_ids);
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}
	ASSERT0P(zp);

	if ((error = zfs_zaccess(dzp, ACE_ADD_SUBDIRECTORY, 0, B_FALSE, cr,
	    mnt_ns))) {
		zfs_acl_ids_free(&acl_ids);
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	if (zfs_acl_ids_overquota(zfsvfs, &acl_ids, zfs_inherit_projid(dzp))) {
		zfs_acl_ids_free(&acl_ids);
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EDQUOT));
	}

	/*
	 * Add a new entry to the directory.
	 */
	getnewvnode_reserve();
	tx = dmu_tx_create(zfsvfs->z_os);
	dmu_tx_hold_zap(tx, dzp->z_id, TRUE, dirname);
	dmu_tx_hold_zap(tx, DMU_NEW_OBJECT, FALSE, NULL);
	fuid_dirtied = zfsvfs->z_fuid_dirty;
	if (fuid_dirtied)
		zfs_fuid_txhold(zfsvfs, tx);
	if (!zfsvfs->z_use_sa && acl_ids.z_aclp->z_acl_bytes > ZFS_ACE_SPACE) {
		dmu_tx_hold_write(tx, DMU_NEW_OBJECT, 0,
		    acl_ids.z_aclp->z_acl_bytes);
	}

	dmu_tx_hold_sa_create(tx, acl_ids.z_aclp->z_acl_bytes +
	    ZFS_SA_BASE_ATTR_SIZE);

	error = dmu_tx_assign(tx, DMU_TX_WAIT);
	if (error) {
		zfs_acl_ids_free(&acl_ids);
		dmu_tx_abort(tx);
		getnewvnode_drop_reserve();
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	/*
	 * Create new node.
	 */
	zfs_mknode(dzp, vap, tx, cr, 0, &zp, &acl_ids);

	/*
	 * Now put new name in parent dir.
	 */
	error = zfs_link_create(dzp, dirname, zp, tx, ZNEW);
	if (error != 0) {
		zfs_znode_delete(zp, tx);
		VOP_UNLOCK(ZTOV(zp));
		zrele(zp);
		goto out;
	}

	if (fuid_dirtied)
		zfs_fuid_sync(zfsvfs, tx);

	*zpp = zp;

	txtype = zfs_log_create_txtype(Z_DIR, NULL, vap);
	zfs_log_create(zilog, tx, txtype, dzp, zp, dirname, NULL,
	    acl_ids.z_fuidp, vap);

out:
	zfs_acl_ids_free(&acl_ids);

	dmu_tx_commit(tx);

	getnewvnode_drop_reserve();

	if (error == 0 && zfsvfs->z_os->os_sync == ZFS_SYNC_ALWAYS)
		error = zil_commit(zilog, 0);

	zfs_exit(zfsvfs, FTAG);
	return (error);
}

/*
 * Remove a directory subdir entry.  If the current working
 * directory is the same as the subdir to be removed, the
 * remove will fail.
 *
 *	IN:	dvp	- vnode of directory to remove from.
 *		name	- name of directory to be removed.
 *		cwd	- vnode of current working directory.
 *		cr	- credentials of caller.
 *		ct	- caller context
 *		flags	- case flags
 *
 *	RETURN:	0 on success, error code on failure.
 *
 * Timestamps:
 *	dvp - ctime|mtime updated
 */
static int
zfs_rmdir_(vnode_t *dvp, vnode_t *vp, const char *name, cred_t *cr)
{
	znode_t		*dzp = VTOZ(dvp);
	znode_t		*zp = VTOZ(vp);
	zfsvfs_t	*zfsvfs = dzp->z_zfsvfs;
	zilog_t		*zilog;
	dmu_tx_t	*tx;
	int		error;

	if ((error = zfs_enter_verify_zp(zfsvfs, dzp, FTAG)) != 0)
		return (error);
	if ((error = zfs_verify_zp(zp)) != 0) {
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}
	zilog = zfsvfs->z_log;


	if ((error = zfs_zaccess_delete(dzp, zp, cr, NULL))) {
		goto out;
	}

	if (vp->v_type != VDIR) {
		error = SET_ERROR(ENOTDIR);
		goto out;
	}

	vnevent_rmdir(vp, dvp, name, ct);

	tx = dmu_tx_create(zfsvfs->z_os);
	dmu_tx_hold_zap(tx, dzp->z_id, FALSE, name);
	dmu_tx_hold_sa(tx, zp->z_sa_hdl, B_FALSE);
	dmu_tx_hold_zap(tx, zfsvfs->z_unlinkedobj, FALSE, NULL);
	zfs_sa_upgrade_txholds(tx, zp);
	zfs_sa_upgrade_txholds(tx, dzp);
	dmu_tx_mark_netfree(tx);
	error = dmu_tx_assign(tx, DMU_TX_WAIT);
	if (error) {
		dmu_tx_abort(tx);
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	error = zfs_link_destroy(dzp, name, zp, tx, ZEXISTS, NULL);

	if (error == 0) {
		uint64_t txtype = TX_RMDIR;
		zfs_log_remove(zilog, tx, txtype, dzp, name,
		    ZFS_NO_OBJECT, B_FALSE);
	}

	dmu_tx_commit(tx);

	if (zfsvfs->z_use_namecache)
		cache_vop_rmdir(dvp, vp);
out:
	if (error == 0 && zfsvfs->z_os->os_sync == ZFS_SYNC_ALWAYS)
		error = zil_commit(zilog, 0);

	zfs_exit(zfsvfs, FTAG);
	return (error);
}

int
zfs_rmdir(znode_t *dzp, const char *name, znode_t *cwd, cred_t *cr, int flags)
{
	struct componentname cn;
	vnode_t *vp;
	int error;

	if ((error = zfs_lookup_internal(dzp, name, &vp, &cn, DELETE)))
		return (error);

	error = zfs_rmdir_(ZTOV(dzp), vp, name, cr);
	vput(vp);
	return (error);
}

/*
 * Read as many directory entries as will fit into the provided
 * buffer from the given directory cursor position (specified in
 * the uio structure).
 *
 *	IN:	vp	- vnode of directory to read.
 *		uio	- structure supplying read location, range info,
 *			  and return buffer.
 *		cr	- credentials of caller.
 *		ct	- caller context
 *
 *	OUT:	uio	- updated offset and range, buffer filled.
 *		eofp	- set to true if end-of-file detected.
 *		ncookies- number of entries in cookies
 *		cookies	- offsets to directory entries
 *
 *	RETURN:	0 on success, error code on failure.
 *
 * Timestamps:
 *	vp - atime updated
 *
 * Note that the low 4 bits of the cookie returned by zap is always zero.
 * This allows us to use the low range for "special" directory entries:
 * We use 0 for '.', and 1 for '..'.  If this is the root of the filesystem,
 * we use the offset 2 for the '.zfs' directory.
 */

/*
 * Get the requested file attributes and place them in the provided
 * vattr structure.
 *
 *	IN:	vp	- vnode of file.
 *		vap	- va_mask identifies requested attributes.
 *			  If AT_XVATTR set, then optional attrs are requested
 *		flags	- ATTR_NOACLCHECK (CIFS server context)
 *		cr	- credentials of caller.
 *
 *	OUT:	vap	- attribute values.
 *
 *	RETURN:	0 (always succeeds).
 */
static int
zfs_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
{
	znode_t *zp = VTOZ(vp);
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	int	error = 0;
	uint32_t blksize;
	u_longlong_t nblocks;
	uint64_t mtime[2], ctime[2], crtime[2], rdev;
	xvattr_t *xvap = (xvattr_t *)vap;	/* vap may be an xvattr_t * */
	xoptattr_t *xoap = NULL;
	boolean_t skipaclchk = (flags & ATTR_NOACLCHECK) ? B_TRUE : B_FALSE;
	sa_bulk_attr_t bulk[4];
	int count = 0;

	if ((error = zfs_enter_verify_zp(zfsvfs, zp, FTAG)) != 0)
		return (error);

	zfs_fuid_map_ids(zp, cr, &vap->va_uid, &vap->va_gid);

	SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_MTIME(zfsvfs), NULL, &mtime, 16);
	SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_CTIME(zfsvfs), NULL, &ctime, 16);
	SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_CRTIME(zfsvfs), NULL, &crtime, 16);
	if (vp->v_type == VBLK || vp->v_type == VCHR)
		SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_RDEV(zfsvfs), NULL,
		    &rdev, 8);

	if ((error = sa_bulk_lookup(zp->z_sa_hdl, bulk, count)) != 0) {
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	/*
	 * If ACL is trivial don't bother looking for ACE_READ_ATTRIBUTES.
	 * Also, if we are the owner don't bother, since owner should
	 * always be allowed to read basic attributes of file.
	 */
	if (!(zp->z_pflags & ZFS_ACL_TRIVIAL) &&
	    (vap->va_uid != crgetuid(cr))) {
		if ((error = zfs_zaccess(zp, ACE_READ_ATTRIBUTES, 0,
		    skipaclchk, cr, NULL))) {
			zfs_exit(zfsvfs, FTAG);
			return (error);
		}
	}

	/*
	 * Return all attributes.  It's cheaper to provide the answer
	 * than to determine whether we were asked the question.
	 */

	vap->va_type = IFTOVT(zp->z_mode);
	vap->va_mode = zp->z_mode & ~S_IFMT;
	vap->va_fsid = vp->v_mount->mnt_stat.f_fsid;
	vap->va_nodeid = zp->z_id;
	vap->va_nlink = zp->z_links;
	if ((vp->v_flag & VROOT) && zfs_show_ctldir(zp) &&
	    zp->z_links < ZFS_LINK_MAX)
		vap->va_nlink++;
	vap->va_size = zp->z_size;
	if (vp->v_type == VBLK || vp->v_type == VCHR)
		vap->va_rdev = zfs_cmpldev(rdev);
	else
		vap->va_rdev = NODEV;
	vap->va_gen = zp->z_gen;
	vap->va_flags = 0;	/* FreeBSD: Reset chflags(2) flags. */
	vap->va_filerev = zp->z_seq;

	/*
	 * Add in any requested optional attributes and the create time.
	 * Also set the corresponding bits in the returned attribute bitmap.
	 */
	if ((xoap = xva_getxoptattr(xvap)) != NULL && zfsvfs->z_use_fuids) {
		if (XVA_ISSET_REQ(xvap, XAT_ARCHIVE)) {
			xoap->xoa_archive =
			    ((zp->z_pflags & ZFS_ARCHIVE) != 0);
			XVA_SET_RTN(xvap, XAT_ARCHIVE);
		}

		if (XVA_ISSET_REQ(xvap, XAT_READONLY)) {
			xoap->xoa_readonly =
			    ((zp->z_pflags & ZFS_READONLY) != 0);
			XVA_SET_RTN(xvap, XAT_READONLY);
		}

		if (XVA_ISSET_REQ(xvap, XAT_SYSTEM)) {
			xoap->xoa_system =
			    ((zp->z_pflags & ZFS_SYSTEM) != 0);
			XVA_SET_RTN(xvap, XAT_SYSTEM);
		}

		if (XVA_ISSET_REQ(xvap, XAT_HIDDEN)) {
			xoap->xoa_hidden =
			    ((zp->z_pflags & ZFS_HIDDEN) != 0);
			XVA_SET_RTN(xvap, XAT_HIDDEN);
		}

		if (XVA_ISSET_REQ(xvap, XAT_NOUNLINK)) {
			xoap->xoa_nounlink =
			    ((zp->z_pflags & ZFS_NOUNLINK) != 0);
			XVA_SET_RTN(xvap, XAT_NOUNLINK);
		}

		if (XVA_ISSET_REQ(xvap, XAT_IMMUTABLE)) {
			xoap->xoa_immutable =
			    ((zp->z_pflags & ZFS_IMMUTABLE) != 0);
			XVA_SET_RTN(xvap, XAT_IMMUTABLE);
		}

		if (XVA_ISSET_REQ(xvap, XAT_APPENDONLY)) {
			xoap->xoa_appendonly =
			    ((zp->z_pflags & ZFS_APPENDONLY) != 0);
			XVA_SET_RTN(xvap, XAT_APPENDONLY);
		}

		if (XVA_ISSET_REQ(xvap, XAT_NODUMP)) {
			xoap->xoa_nodump =
			    ((zp->z_pflags & ZFS_NODUMP) != 0);
			XVA_SET_RTN(xvap, XAT_NODUMP);
		}

		if (XVA_ISSET_REQ(xvap, XAT_OPAQUE)) {
			xoap->xoa_opaque =
			    ((zp->z_pflags & ZFS_OPAQUE) != 0);
			XVA_SET_RTN(xvap, XAT_OPAQUE);
		}

		if (XVA_ISSET_REQ(xvap, XAT_AV_QUARANTINED)) {
			xoap->xoa_av_quarantined =
			    ((zp->z_pflags & ZFS_AV_QUARANTINED) != 0);
			XVA_SET_RTN(xvap, XAT_AV_QUARANTINED);
		}

		if (XVA_ISSET_REQ(xvap, XAT_AV_MODIFIED)) {
			xoap->xoa_av_modified =
			    ((zp->z_pflags & ZFS_AV_MODIFIED) != 0);
			XVA_SET_RTN(xvap, XAT_AV_MODIFIED);
		}

		if (XVA_ISSET_REQ(xvap, XAT_AV_SCANSTAMP) &&
		    vp->v_type == VREG) {
			zfs_sa_get_scanstamp(zp, xvap);
		}

		if (XVA_ISSET_REQ(xvap, XAT_REPARSE)) {
			xoap->xoa_reparse = ((zp->z_pflags & ZFS_REPARSE) != 0);
			XVA_SET_RTN(xvap, XAT_REPARSE);
		}
		if (XVA_ISSET_REQ(xvap, XAT_GEN)) {
			xoap->xoa_generation = zp->z_gen;
			XVA_SET_RTN(xvap, XAT_GEN);
		}

		if (XVA_ISSET_REQ(xvap, XAT_OFFLINE)) {
			xoap->xoa_offline =
			    ((zp->z_pflags & ZFS_OFFLINE) != 0);
			XVA_SET_RTN(xvap, XAT_OFFLINE);
		}

		if (XVA_ISSET_REQ(xvap, XAT_SPARSE)) {
			xoap->xoa_sparse =
			    ((zp->z_pflags & ZFS_SPARSE) != 0);
			XVA_SET_RTN(xvap, XAT_SPARSE);
		}

		if (XVA_ISSET_REQ(xvap, XAT_PROJINHERIT)) {
			xoap->xoa_projinherit =
			    ((zp->z_pflags & ZFS_PROJINHERIT) != 0);
			XVA_SET_RTN(xvap, XAT_PROJINHERIT);
		}

		if (XVA_ISSET_REQ(xvap, XAT_PROJID)) {
			xoap->xoa_projid = zp->z_projid;
			XVA_SET_RTN(xvap, XAT_PROJID);
		}
	}

	ZFS_TIME_DECODE(&vap->va_atime, zp->z_atime);
	ZFS_TIME_DECODE(&vap->va_mtime, mtime);
	ZFS_TIME_DECODE(&vap->va_ctime, ctime);
	ZFS_TIME_DECODE(&vap->va_birthtime, crtime);


	sa_object_size(zp->z_sa_hdl, &blksize, &nblocks);
	vap->va_blksize = blksize;
	vap->va_bytes = nblocks << 9;	/* nblocks * 512 */

	if (zp->z_blksz == 0) {
		/*
		 * Block size hasn't been set; suggest maximal I/O transfers.
		 */
		vap->va_blksize = zfsvfs->z_max_blksz;
	}

	zfs_exit(zfsvfs, FTAG);
	return (0);
}

/*
 * For the operation of changing file's user/group/project, we need to
 * handle not only the main object that is assigned to the file directly,
 * but also the ones that are used by the file via hidden xattr directory.
 *
 * Because the xattr directory may contains many EA entries, as to it may
 * be impossible to change all of them via the transaction of changing the
 * main object's user/group/project attributes. Then we have to change them
 * via other multiple independent transactions one by one. It may be not good
 * solution, but we have no better idea yet.
 */
static int
zfs_setattr_dir(znode_t *dzp)
{
	zfsvfs_t	*zfsvfs = dzp->z_zfsvfs;
	objset_t	*os = zfsvfs->z_os;
	zap_cursor_t	zc;
	zap_attribute_t	*zap;
	znode_t		*zp = NULL;
	dmu_tx_t	*tx = NULL;
	uint64_t	uid, gid;
	sa_bulk_attr_t	bulk[4];
	int		count;
	int		err;

	zap = zap_attribute_alloc();
	zap_cursor_init(&zc, os, dzp->z_id);
	while ((err = zap_cursor_retrieve(&zc, zap)) == 0) {
		count = 0;
		if (zap->za_integer_length != 8 || zap->za_num_integers != 1) {
			err = ENXIO;
			break;
		}

		err = zfs_dirent_lookup(dzp, zap->za_name, &zp, ZEXISTS);
		if (err == ENOENT)
			goto next;
		if (err)
			break;

		if (zp->z_uid == dzp->z_uid &&
		    zp->z_gid == dzp->z_gid &&
		    zp->z_projid == dzp->z_projid)
			goto next;

		tx = dmu_tx_create(os);
		if (!(zp->z_pflags & ZFS_PROJID))
			dmu_tx_hold_sa(tx, zp->z_sa_hdl, B_TRUE);
		else
			dmu_tx_hold_sa(tx, zp->z_sa_hdl, B_FALSE);

		err = dmu_tx_assign(tx, DMU_TX_WAIT);
		if (err)
			break;

		vn_seqc_write_begin(ZTOV(zp));
		mutex_enter(&dzp->z_lock);

		if (zp->z_uid != dzp->z_uid) {
			uid = dzp->z_uid;
			SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_UID(zfsvfs), NULL,
			    &uid, sizeof (uid));
			zp->z_uid = uid;
		}

		if (zp->z_gid != dzp->z_gid) {
			gid = dzp->z_gid;
			SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_GID(zfsvfs), NULL,
			    &gid, sizeof (gid));
			zp->z_gid = gid;
		}

		uint64_t projid = dzp->z_projid;
		if (zp->z_projid != projid) {
			if (!(zp->z_pflags & ZFS_PROJID)) {
				err = sa_add_projid(zp->z_sa_hdl, tx, projid);
				if (unlikely(err == EEXIST)) {
					err = 0;
				} else if (err != 0) {
					goto sa_add_projid_err;
				} else {
					projid = ZFS_INVALID_PROJID;
				}
			}

			if (projid != ZFS_INVALID_PROJID) {
				zp->z_projid = projid;
				SA_ADD_BULK_ATTR(bulk, count,
				    SA_ZPL_PROJID(zfsvfs), NULL, &zp->z_projid,
				    sizeof (zp->z_projid));
			}
		}

sa_add_projid_err:
		mutex_exit(&dzp->z_lock);

		if (likely(count > 0)) {
			err = sa_bulk_update(zp->z_sa_hdl, bulk, count, tx);
			dmu_tx_commit(tx);
		} else if (projid == ZFS_INVALID_PROJID) {
			dmu_tx_commit(tx);
		} else {
			dmu_tx_abort(tx);
		}
		tx = NULL;
		vn_seqc_write_end(ZTOV(zp));
		if (err != 0 && err != ENOENT)
			break;

next:
		if (zp) {
			zrele(zp);
			zp = NULL;
		}
		zap_cursor_advance(&zc);
	}

	if (tx)
		dmu_tx_abort(tx);
	if (zp) {
		zrele(zp);
	}
	zap_cursor_fini(&zc);
	zap_attribute_free(zap);

	return (err == ENOENT ? 0 : err);
}

/*
 * Set the file attributes to the values contained in the
 * vattr structure.
 *
 *	IN:	zp	- znode of file to be modified.
 *		vap	- new attribute values.
 *			  If AT_XVATTR set, then optional attrs are being set
 *		flags	- ATTR_UTIME set if non-default time values provided.
 *			- ATTR_NOACLCHECK (CIFS context only).
 *		cr	- credentials of caller.
 *		mnt_ns	- Unused on FreeBSD
 *
 *	RETURN:	0 on success, error code on failure.
 *
 * Timestamps:
 *	vp - ctime updated, mtime updated if size changed.
 */
int
zfs_setattr(znode_t *zp, vattr_t *vap, int flags, cred_t *cr, zidmap_t *mnt_ns)
{
	vnode_t		*vp = ZTOV(zp);
	zfsvfs_t	*zfsvfs = zp->z_zfsvfs;
	objset_t	*os;
	zilog_t		*zilog;
	dmu_tx_t	*tx;
	vattr_t		oldva;
	xvattr_t	tmpxvattr;
	uint_t		mask = vap->va_mask;
	uint_t		saved_mask = 0;
	uint64_t	saved_mode;
	int		trim_mask = 0;
	uint64_t	new_mode;
	uint64_t	new_uid, new_gid;
	uint64_t	xattr_obj;
	uint64_t	mtime[2], ctime[2];
	uint64_t	projid = ZFS_INVALID_PROJID;
	znode_t		*attrzp;
	int		need_policy = FALSE;
	int		err, err2;
	zfs_fuid_info_t *fuidp = NULL;
	xvattr_t *xvap = (xvattr_t *)vap;	/* vap may be an xvattr_t * */
	xoptattr_t	*xoap;
	zfs_acl_t	*aclp;
	boolean_t skipaclchk = (flags & ATTR_NOACLCHECK) ? B_TRUE : B_FALSE;
	boolean_t	fuid_dirtied = B_FALSE;
	boolean_t	handle_eadir = B_FALSE;
	sa_bulk_attr_t	bulk[7], xattr_bulk[7];
	int		count = 0, xattr_count = 0;

	if (mask == 0)
		return (0);

	if (mask & AT_NOSET)
		return (SET_ERROR(EINVAL));

	if ((err = zfs_enter_verify_zp(zfsvfs, zp, FTAG)) != 0)
		return (err);

	os = zfsvfs->z_os;
	zilog = zfsvfs->z_log;

	/*
	 * Make sure that if we have ephemeral uid/gid or xvattr specified
	 * that file system is at proper version level
	 */

	if (zfsvfs->z_use_fuids == B_FALSE &&
	    (((mask & AT_UID) && IS_EPHEMERAL(vap->va_uid)) ||
	    ((mask & AT_GID) && IS_EPHEMERAL(vap->va_gid)) ||
	    (mask & AT_XVATTR))) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EINVAL));
	}

	if (mask & AT_SIZE && vp->v_type == VDIR) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EISDIR));
	}

	if (mask & AT_SIZE && vp->v_type != VREG && vp->v_type != VFIFO) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EINVAL));
	}

	/*
	 * If this is an xvattr_t, then get a pointer to the structure of
	 * optional attributes.  If this is NULL, then we have a vattr_t.
	 */
	xoap = xva_getxoptattr(xvap);

	xva_init(&tmpxvattr);

	/*
	 * Immutable files can only alter immutable bit and atime
	 */
	if ((zp->z_pflags & ZFS_IMMUTABLE) &&
	    ((mask & (AT_SIZE|AT_UID|AT_GID|AT_MTIME|AT_MODE)) ||
	    ((mask & AT_XVATTR) && XVA_ISSET_REQ(xvap, XAT_CREATETIME)))) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EPERM));
	}

	/*
	 * Note: ZFS_READONLY is handled in zfs_zaccess_common.
	 */

	/*
	 * Verify timestamps doesn't overflow 32 bits.
	 * ZFS can handle large timestamps, but 32bit syscalls can't
	 * handle times greater than 2039.  This check should be removed
	 * once large timestamps are fully supported.
	 */
	if (mask & (AT_ATIME | AT_MTIME)) {
		if (((mask & AT_ATIME) && TIMESPEC_OVERFLOW(&vap->va_atime)) ||
		    ((mask & AT_MTIME) && TIMESPEC_OVERFLOW(&vap->va_mtime))) {
			zfs_exit(zfsvfs, FTAG);
			return (SET_ERROR(EOVERFLOW));
		}
	}
	if (xoap != NULL && (mask & AT_XVATTR)) {
		if (XVA_ISSET_REQ(xvap, XAT_CREATETIME) &&
		    TIMESPEC_OVERFLOW(&vap->va_birthtime)) {
			zfs_exit(zfsvfs, FTAG);
			return (SET_ERROR(EOVERFLOW));
		}

		if (XVA_ISSET_REQ(xvap, XAT_PROJID)) {
			if (!dmu_objset_projectquota_enabled(os) ||
			    (!S_ISREG(zp->z_mode) && !S_ISDIR(zp->z_mode))) {
				zfs_exit(zfsvfs, FTAG);
				return (SET_ERROR(EOPNOTSUPP));
			}

			projid = xoap->xoa_projid;
			if (unlikely(projid == ZFS_INVALID_PROJID)) {
				zfs_exit(zfsvfs, FTAG);
				return (SET_ERROR(EINVAL));
			}

			if (projid == zp->z_projid && zp->z_pflags & ZFS_PROJID)
				projid = ZFS_INVALID_PROJID;
			else
				need_policy = TRUE;
		}

		if (XVA_ISSET_REQ(xvap, XAT_PROJINHERIT) &&
		    (xoap->xoa_projinherit !=
		    ((zp->z_pflags & ZFS_PROJINHERIT) != 0)) &&
		    (!dmu_objset_projectquota_enabled(os) ||
		    (!S_ISREG(zp->z_mode) && !S_ISDIR(zp->z_mode)))) {
			zfs_exit(zfsvfs, FTAG);
			return (SET_ERROR(EOPNOTSUPP));
		}
	}

	attrzp = NULL;
	aclp = NULL;

	if (zfsvfs->z_vfs->vfs_flag & VFS_RDONLY) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EROFS));
	}

	/*
	 * First validate permissions
	 */

	if (mask & AT_SIZE) {
		/*
		 * XXX - Note, we are not providing any open
		 * mode flags here (like FNDELAY), so we may
		 * block if there are locks present... this
		 * should be addressed in openat().
		 */
		/* XXX - would it be OK to generate a log record here? */
		err = zfs_freesp(zp, vap->va_size, 0, 0, FALSE);
		if (err) {
			zfs_exit(zfsvfs, FTAG);
			return (err);
		}
	}

	if (mask & (AT_ATIME|AT_MTIME) ||
	    ((mask & AT_XVATTR) && (XVA_ISSET_REQ(xvap, XAT_HIDDEN) ||
	    XVA_ISSET_REQ(xvap, XAT_READONLY) ||
	    XVA_ISSET_REQ(xvap, XAT_ARCHIVE) ||
	    XVA_ISSET_REQ(xvap, XAT_OFFLINE) ||
	    XVA_ISSET_REQ(xvap, XAT_SPARSE) ||
	    XVA_ISSET_REQ(xvap, XAT_CREATETIME) ||
	    XVA_ISSET_REQ(xvap, XAT_SYSTEM)))) {
		need_policy = zfs_zaccess(zp, ACE_WRITE_ATTRIBUTES, 0,
		    skipaclchk, cr, mnt_ns);
	}

	if (mask & (AT_UID|AT_GID)) {
		int	idmask = (mask & (AT_UID|AT_GID));
		int	take_owner;
		int	take_group;

		/*
		 * NOTE: even if a new mode is being set,
		 * we may clear S_ISUID/S_ISGID bits.
		 */

		if (!(mask & AT_MODE))
			vap->va_mode = zp->z_mode;

		/*
		 * Take ownership or chgrp to group we are a member of
		 */

		take_owner = (mask & AT_UID) && (vap->va_uid == crgetuid(cr));
		take_group = (mask & AT_GID) &&
		    zfs_groupmember(zfsvfs, vap->va_gid, cr);

		/*
		 * If both AT_UID and AT_GID are set then take_owner and
		 * take_group must both be set in order to allow taking
		 * ownership.
		 *
		 * Otherwise, send the check through secpolicy_vnode_setattr()
		 *
		 */

		if (((idmask == (AT_UID|AT_GID)) && take_owner && take_group) ||
		    ((idmask == AT_UID) && take_owner) ||
		    ((idmask == AT_GID) && take_group)) {
			if (zfs_zaccess(zp, ACE_WRITE_OWNER, 0,
			    skipaclchk, cr, mnt_ns) == 0) {
				/*
				 * Remove setuid/setgid for non-privileged users
				 */
				secpolicy_setid_clear(vap, vp, cr);
				trim_mask = (mask & (AT_UID|AT_GID));
			} else {
				need_policy =  TRUE;
			}
		} else {
			need_policy =  TRUE;
		}
	}

	oldva.va_mode = zp->z_mode;
	zfs_fuid_map_ids(zp, cr, &oldva.va_uid, &oldva.va_gid);
	if (mask & AT_XVATTR) {
		/*
		 * Update xvattr mask to include only those attributes
		 * that are actually changing.
		 *
		 * the bits will be restored prior to actually setting
		 * the attributes so the caller thinks they were set.
		 */
		if (XVA_ISSET_REQ(xvap, XAT_APPENDONLY)) {
			if (xoap->xoa_appendonly !=
			    ((zp->z_pflags & ZFS_APPENDONLY) != 0)) {
				need_policy = TRUE;
			} else {
				XVA_CLR_REQ(xvap, XAT_APPENDONLY);
				XVA_SET_REQ(&tmpxvattr, XAT_APPENDONLY);
			}
		}

		if (XVA_ISSET_REQ(xvap, XAT_PROJINHERIT)) {
			if (xoap->xoa_projinherit !=
			    ((zp->z_pflags & ZFS_PROJINHERIT) != 0)) {
				need_policy = TRUE;
			} else {
				XVA_CLR_REQ(xvap, XAT_PROJINHERIT);
				XVA_SET_REQ(&tmpxvattr, XAT_PROJINHERIT);
			}
		}

		if (XVA_ISSET_REQ(xvap, XAT_NOUNLINK)) {
			if (xoap->xoa_nounlink !=
			    ((zp->z_pflags & ZFS_NOUNLINK) != 0)) {
				need_policy = TRUE;
			} else {
				XVA_CLR_REQ(xvap, XAT_NOUNLINK);
				XVA_SET_REQ(&tmpxvattr, XAT_NOUNLINK);
			}
		}

		if (XVA_ISSET_REQ(xvap, XAT_IMMUTABLE)) {
			if (xoap->xoa_immutable !=
			    ((zp->z_pflags & ZFS_IMMUTABLE) != 0)) {
				need_policy = TRUE;
			} else {
				XVA_CLR_REQ(xvap, XAT_IMMUTABLE);
				XVA_SET_REQ(&tmpxvattr, XAT_IMMUTABLE);
			}
		}

		if (XVA_ISSET_REQ(xvap, XAT_NODUMP)) {
			if (xoap->xoa_nodump !=
			    ((zp->z_pflags & ZFS_NODUMP) != 0)) {
				need_policy = TRUE;
			} else {
				XVA_CLR_REQ(xvap, XAT_NODUMP);
				XVA_SET_REQ(&tmpxvattr, XAT_NODUMP);
			}
		}

		if (XVA_ISSET_REQ(xvap, XAT_AV_MODIFIED)) {
			if (xoap->xoa_av_modified !=
			    ((zp->z_pflags & ZFS_AV_MODIFIED) != 0)) {
				need_policy = TRUE;
			} else {
				XVA_CLR_REQ(xvap, XAT_AV_MODIFIED);
				XVA_SET_REQ(&tmpxvattr, XAT_AV_MODIFIED);
			}
		}

		if (XVA_ISSET_REQ(xvap, XAT_AV_QUARANTINED)) {
			if ((vp->v_type != VREG &&
			    xoap->xoa_av_quarantined) ||
			    xoap->xoa_av_quarantined !=
			    ((zp->z_pflags & ZFS_AV_QUARANTINED) != 0)) {
				need_policy = TRUE;
			} else {
				XVA_CLR_REQ(xvap, XAT_AV_QUARANTINED);
				XVA_SET_REQ(&tmpxvattr, XAT_AV_QUARANTINED);
			}
		}

		if (XVA_ISSET_REQ(xvap, XAT_REPARSE)) {
			zfs_exit(zfsvfs, FTAG);
			return (SET_ERROR(EPERM));
		}

		if (need_policy == FALSE &&
		    (XVA_ISSET_REQ(xvap, XAT_AV_SCANSTAMP) ||
		    XVA_ISSET_REQ(xvap, XAT_OPAQUE))) {
			need_policy = TRUE;
		}
	}

	if (mask & AT_MODE) {
		if (zfs_zaccess(zp, ACE_WRITE_ACL, 0, skipaclchk, cr,
		    mnt_ns) == 0) {
			err = secpolicy_setid_setsticky_clear(vp, vap,
			    &oldva, cr);
			if (err) {
				zfs_exit(zfsvfs, FTAG);
				return (err);
			}
			trim_mask |= AT_MODE;
		} else {
			need_policy = TRUE;
		}
	}

	if (need_policy) {
		/*
		 * If trim_mask is set then take ownership
		 * has been granted or write_acl is present and user
		 * has the ability to modify mode.  In that case remove
		 * UID|GID and or MODE from mask so that
		 * secpolicy_vnode_setattr() doesn't revoke it.
		 */

		if (trim_mask) {
			saved_mask = vap->va_mask;
			vap->va_mask &= ~trim_mask;
			if (trim_mask & AT_MODE) {
				/*
				 * Save the mode, as secpolicy_vnode_setattr()
				 * will overwrite it with ova.va_mode.
				 */
				saved_mode = vap->va_mode;
			}
		}
		err = secpolicy_vnode_setattr(cr, vp, vap, &oldva, flags,
		    (int (*)(void *, int, cred_t *))zfs_zaccess_unix, zp);
		if (err) {
			zfs_exit(zfsvfs, FTAG);
			return (err);
		}

		if (trim_mask) {
			vap->va_mask |= saved_mask;
			if (trim_mask & AT_MODE) {
				/*
				 * Recover the mode after
				 * secpolicy_vnode_setattr().
				 */
				vap->va_mode = saved_mode;
			}
		}
	}

	/*
	 * secpolicy_vnode_setattr, or take ownership may have
	 * changed va_mask
	 */
	mask = vap->va_mask;

	if ((mask & (AT_UID | AT_GID)) || projid != ZFS_INVALID_PROJID) {
		handle_eadir = B_TRUE;
		err = sa_lookup(zp->z_sa_hdl, SA_ZPL_XATTR(zfsvfs),
		    &xattr_obj, sizeof (xattr_obj));

		if (err == 0 && xattr_obj) {
			err = zfs_zget(zp->z_zfsvfs, xattr_obj, &attrzp);
			if (err == 0) {
				err = vn_lock(ZTOV(attrzp), LK_EXCLUSIVE);
				if (err != 0)
					vrele(ZTOV(attrzp));
			}
			if (err)
				goto out2;
		}
		if (mask & AT_UID) {
			new_uid = zfs_fuid_create(zfsvfs,
			    (uint64_t)vap->va_uid, cr, ZFS_OWNER, &fuidp);
			if (new_uid != zp->z_uid &&
			    zfs_id_overquota(zfsvfs, DMU_USERUSED_OBJECT,
			    new_uid)) {
				if (attrzp)
					vput(ZTOV(attrzp));
				err = SET_ERROR(EDQUOT);
				goto out2;
			}
		}

		if (mask & AT_GID) {
			new_gid = zfs_fuid_create(zfsvfs, (uint64_t)vap->va_gid,
			    cr, ZFS_GROUP, &fuidp);
			if (new_gid != zp->z_gid &&
			    zfs_id_overquota(zfsvfs, DMU_GROUPUSED_OBJECT,
			    new_gid)) {
				if (attrzp)
					vput(ZTOV(attrzp));
				err = SET_ERROR(EDQUOT);
				goto out2;
			}
		}

		if (projid != ZFS_INVALID_PROJID &&
		    zfs_id_overquota(zfsvfs, DMU_PROJECTUSED_OBJECT, projid)) {
			if (attrzp)
				vput(ZTOV(attrzp));
			err = SET_ERROR(EDQUOT);
			goto out2;
		}
	}
	tx = dmu_tx_create(os);

	if (mask & AT_MODE) {
		uint64_t pmode = zp->z_mode;
		uint64_t acl_obj;
		new_mode = (pmode & S_IFMT) | (vap->va_mode & ~S_IFMT);

		if (zp->z_zfsvfs->z_acl_mode == ZFS_ACL_RESTRICTED &&
		    !(zp->z_pflags & ZFS_ACL_TRIVIAL)) {
			err = SET_ERROR(EPERM);
			goto out;
		}

		if ((err = zfs_acl_chmod_setattr(zp, &aclp, new_mode)))
			goto out;

		if (!zp->z_is_sa && ((acl_obj = zfs_external_acl(zp)) != 0)) {
			/*
			 * Are we upgrading ACL from old V0 format
			 * to V1 format?
			 */
			if (zfsvfs->z_version >= ZPL_VERSION_FUID &&
			    zfs_znode_acl_version(zp) ==
			    ZFS_ACL_VERSION_INITIAL) {
				dmu_tx_hold_free(tx, acl_obj, 0,
				    DMU_OBJECT_END);
				dmu_tx_hold_write(tx, DMU_NEW_OBJECT,
				    0, aclp->z_acl_bytes);
			} else {
				dmu_tx_hold_write(tx, acl_obj, 0,
				    aclp->z_acl_bytes);
			}
		} else if (!zp->z_is_sa && aclp->z_acl_bytes > ZFS_ACE_SPACE) {
			dmu_tx_hold_write(tx, DMU_NEW_OBJECT,
			    0, aclp->z_acl_bytes);
		}
		dmu_tx_hold_sa(tx, zp->z_sa_hdl, B_TRUE);
	} else {
		if (((mask & AT_XVATTR) &&
		    XVA_ISSET_REQ(xvap, XAT_AV_SCANSTAMP)) ||
		    (projid != ZFS_INVALID_PROJID &&
		    !(zp->z_pflags & ZFS_PROJID)))
			dmu_tx_hold_sa(tx, zp->z_sa_hdl, B_TRUE);
		else
			dmu_tx_hold_sa(tx, zp->z_sa_hdl, B_FALSE);
	}

	if (attrzp) {
		dmu_tx_hold_sa(tx, attrzp->z_sa_hdl, B_FALSE);
	}

	fuid_dirtied = zfsvfs->z_fuid_dirty;
	if (fuid_dirtied)
		zfs_fuid_txhold(zfsvfs, tx);

	zfs_sa_upgrade_txholds(tx, zp);

	err = dmu_tx_assign(tx, DMU_TX_WAIT);
	if (err)
		goto out;

	count = 0;
	/*
	 * Set each attribute requested.
	 * We group settings according to the locks they need to acquire.
	 *
	 * Note: you cannot set ctime directly, although it will be
	 * updated as a side-effect of calling this function.
	 */

	if (projid != ZFS_INVALID_PROJID && !(zp->z_pflags & ZFS_PROJID)) {
		/*
		 * For the existed object that is upgraded from old system,
		 * its on-disk layout has no slot for the project ID attribute.
		 * But quota accounting logic needs to access related slots by
		 * offset directly. So we need to adjust old objects' layout
		 * to make the project ID to some unified and fixed offset.
		 */
		if (attrzp)
			err = sa_add_projid(attrzp->z_sa_hdl, tx, projid);
		if (err == 0)
			err = sa_add_projid(zp->z_sa_hdl, tx, projid);

		if (unlikely(err == EEXIST))
			err = 0;
		else if (err != 0)
			goto out;
		else
			projid = ZFS_INVALID_PROJID;
	}

	if (mask & (AT_UID|AT_GID|AT_MODE))
		mutex_enter(&zp->z_acl_lock);

	SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_FLAGS(zfsvfs), NULL,
	    &zp->z_pflags, sizeof (zp->z_pflags));

	if (attrzp) {
		if (mask & (AT_UID|AT_GID|AT_MODE))
			mutex_enter(&attrzp->z_acl_lock);
		SA_ADD_BULK_ATTR(xattr_bulk, xattr_count,
		    SA_ZPL_FLAGS(zfsvfs), NULL, &attrzp->z_pflags,
		    sizeof (attrzp->z_pflags));
		if (projid != ZFS_INVALID_PROJID) {
			attrzp->z_projid = projid;
			SA_ADD_BULK_ATTR(xattr_bulk, xattr_count,
			    SA_ZPL_PROJID(zfsvfs), NULL, &attrzp->z_projid,
			    sizeof (attrzp->z_projid));
		}
	}

	if (mask & (AT_UID|AT_GID)) {

		if (mask & AT_UID) {
			SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_UID(zfsvfs), NULL,
			    &new_uid, sizeof (new_uid));
			zp->z_uid = new_uid;
			if (attrzp) {
				SA_ADD_BULK_ATTR(xattr_bulk, xattr_count,
				    SA_ZPL_UID(zfsvfs), NULL, &new_uid,
				    sizeof (new_uid));
				attrzp->z_uid = new_uid;
			}
		}

		if (mask & AT_GID) {
			SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_GID(zfsvfs),
			    NULL, &new_gid, sizeof (new_gid));
			zp->z_gid = new_gid;
			if (attrzp) {
				SA_ADD_BULK_ATTR(xattr_bulk, xattr_count,
				    SA_ZPL_GID(zfsvfs), NULL, &new_gid,
				    sizeof (new_gid));
				attrzp->z_gid = new_gid;
			}
		}
		if (!(mask & AT_MODE)) {
			SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_MODE(zfsvfs),
			    NULL, &new_mode, sizeof (new_mode));
			new_mode = zp->z_mode;
		}
		err = zfs_acl_chown_setattr(zp);
		ASSERT0(err);
		if (attrzp) {
			vn_seqc_write_begin(ZTOV(attrzp));
			err = zfs_acl_chown_setattr(attrzp);
			vn_seqc_write_end(ZTOV(attrzp));
			ASSERT0(err);
		}
	}

	if (mask & AT_MODE) {
		SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_MODE(zfsvfs), NULL,
		    &new_mode, sizeof (new_mode));
		zp->z_mode = new_mode;
		ASSERT3P(aclp, !=, NULL);
		err = zfs_aclset_common(zp, aclp, cr, tx);
		ASSERT0(err);
		if (zp->z_acl_cached)
			zfs_acl_free(zp->z_acl_cached);
		zp->z_acl_cached = aclp;
		aclp = NULL;
	}


	if (mask & AT_ATIME) {
		ZFS_TIME_ENCODE(&vap->va_atime, zp->z_atime);
		SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_ATIME(zfsvfs), NULL,
		    &zp->z_atime, sizeof (zp->z_atime));
	}

	if (mask & AT_MTIME) {
		ZFS_TIME_ENCODE(&vap->va_mtime, mtime);
		SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_MTIME(zfsvfs), NULL,
		    mtime, sizeof (mtime));
	}

	if (projid != ZFS_INVALID_PROJID) {
		zp->z_projid = projid;
		SA_ADD_BULK_ATTR(bulk, count,
		    SA_ZPL_PROJID(zfsvfs), NULL, &zp->z_projid,
		    sizeof (zp->z_projid));
	}

	/* XXX - shouldn't this be done *before* the ATIME/MTIME checks? */
	if (mask & AT_SIZE && !(mask & AT_MTIME)) {
		SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_MTIME(zfsvfs),
		    NULL, mtime, sizeof (mtime));
		SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_CTIME(zfsvfs), NULL,
		    &ctime, sizeof (ctime));
		zfs_tstamp_update_setup(zp, CONTENT_MODIFIED, mtime, ctime);
	} else if (mask != 0) {
		SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_CTIME(zfsvfs), NULL,
		    &ctime, sizeof (ctime));
		zfs_tstamp_update_setup(zp, STATE_CHANGED, mtime, ctime);
		if (attrzp) {
			SA_ADD_BULK_ATTR(xattr_bulk, xattr_count,
			    SA_ZPL_CTIME(zfsvfs), NULL,
			    &ctime, sizeof (ctime));
			zfs_tstamp_update_setup(attrzp, STATE_CHANGED,
			    mtime, ctime);
		}
	}

	/*
	 * Do this after setting timestamps to prevent timestamp
	 * update from toggling bit
	 */

	if (xoap && (mask & AT_XVATTR)) {

		if (XVA_ISSET_REQ(xvap, XAT_CREATETIME))
			xoap->xoa_createtime = vap->va_birthtime;
		/*
		 * restore trimmed off masks
		 * so that return masks can be set for caller.
		 */

		if (XVA_ISSET_REQ(&tmpxvattr, XAT_APPENDONLY)) {
			XVA_SET_REQ(xvap, XAT_APPENDONLY);
		}
		if (XVA_ISSET_REQ(&tmpxvattr, XAT_NOUNLINK)) {
			XVA_SET_REQ(xvap, XAT_NOUNLINK);
		}
		if (XVA_ISSET_REQ(&tmpxvattr, XAT_IMMUTABLE)) {
			XVA_SET_REQ(xvap, XAT_IMMUTABLE);
		}
		if (XVA_ISSET_REQ(&tmpxvattr, XAT_NODUMP)) {
			XVA_SET_REQ(xvap, XAT_NODUMP);
		}
		if (XVA_ISSET_REQ(&tmpxvattr, XAT_AV_MODIFIED)) {
			XVA_SET_REQ(xvap, XAT_AV_MODIFIED);
		}
		if (XVA_ISSET_REQ(&tmpxvattr, XAT_AV_QUARANTINED)) {
			XVA_SET_REQ(xvap, XAT_AV_QUARANTINED);
		}
		if (XVA_ISSET_REQ(&tmpxvattr, XAT_PROJINHERIT)) {
			XVA_SET_REQ(xvap, XAT_PROJINHERIT);
		}

		if (XVA_ISSET_REQ(xvap, XAT_AV_SCANSTAMP))
			ASSERT3S(vp->v_type, ==, VREG);

		zfs_xvattr_set(zp, xvap, tx);
	}

	if (fuid_dirtied)
		zfs_fuid_sync(zfsvfs, tx);

	if (mask != 0)
		zfs_log_setattr(zilog, tx, TX_SETATTR, zp, vap, mask, fuidp);

	if (mask & (AT_UID|AT_GID|AT_MODE))
		mutex_exit(&zp->z_acl_lock);

	if (attrzp) {
		if (mask & (AT_UID|AT_GID|AT_MODE))
			mutex_exit(&attrzp->z_acl_lock);
	}
out:
	if (err == 0 && attrzp) {
		err2 = sa_bulk_update(attrzp->z_sa_hdl, xattr_bulk,
		    xattr_count, tx);
		ASSERT0(err2);
	}

	if (aclp)
		zfs_acl_free(aclp);

	if (fuidp) {
		zfs_fuid_info_free(fuidp);
		fuidp = NULL;
	}

	if (err) {
		dmu_tx_abort(tx);
		if (attrzp)
			vput(ZTOV(attrzp));
	} else {
		err2 = sa_bulk_update(zp->z_sa_hdl, bulk, count, tx);
		dmu_tx_commit(tx);
		if (attrzp) {
			if (err2 == 0 && handle_eadir)
				err = zfs_setattr_dir(attrzp);
			vput(ZTOV(attrzp));
		}
	}

out2:
	if (err == 0 && os->os_sync == ZFS_SYNC_ALWAYS)
		err = zil_commit(zilog, 0);

	zfs_exit(zfsvfs, FTAG);
	return (err);
}

/*
 * Look up the directory entries corresponding to the source and target
 * directory/name pairs.
 */

static int
zfs_rename_check(znode_t *szp, znode_t *sdzp, znode_t *tdzp)
{
	zfsvfs_t	*zfsvfs;
	znode_t		*zp, *zp1;
	uint64_t	parent;
	int		error;

	zfsvfs = tdzp->z_zfsvfs;
	if (tdzp == szp)
		return (SET_ERROR(EINVAL));
	if (tdzp == sdzp)
		return (0);
	if (tdzp->z_id == zfsvfs->z_root)
		return (0);
	zp = tdzp;
	for (;;) {
		ASSERT(!zp->z_unlinked);
		if ((error = sa_lookup(zp->z_sa_hdl,
		    SA_ZPL_PARENT(zfsvfs), &parent, sizeof (parent))) != 0)
			break;

		if (parent == szp->z_id) {
			error = SET_ERROR(EINVAL);
			break;
		}
		if (parent == zfsvfs->z_root)
			break;
		if (parent == sdzp->z_id)
			break;

		error = zfs_zget(zfsvfs, parent, &zp1);
		if (error != 0)
			break;

		if (zp != tdzp)
			VN_RELE_ASYNC(ZTOV(zp),
			    dsl_pool_zrele_taskq(
			    dmu_objset_pool(zfsvfs->z_os)));
		zp = zp1;
	}

	if (error == ENOTDIR)
		panic("checkpath: .. not a directory\n");
	if (zp != tdzp)
		VN_RELE_ASYNC(ZTOV(zp),
		    dsl_pool_zrele_taskq(dmu_objset_pool(zfsvfs->z_os)));
	return (error);
}

static int
zfs_do_rename_impl(vnode_t *sdvp, vnode_t **svpp, struct componentname *scnp,
    vnode_t *tdvp, vnode_t **tvpp, struct componentname *tcnp,
    cred_t *cr);

/*
 * Move an entry from the provided source directory to the target
 * directory.  Change the entry name as indicated.
 *
 *	IN:	sdvp	- Source directory containing the "old entry".
 *		scnp	- Old entry name.
 *		tdvp	- Target directory to contain the "new entry".
 *		tcnp	- New entry name.
 *		cr	- credentials of caller.
 *	INOUT:	svpp	- Source file
 *		tvpp	- Target file, may point to NULL initially
 *
 *	RETURN:	0 on success, error code on failure.
 *
 * Timestamps:
 *	sdvp,tdvp - ctime|mtime updated
 */

static int
zfs_do_rename_impl(vnode_t *sdvp, vnode_t **svpp, struct componentname *scnp,
    vnode_t *tdvp, vnode_t **tvpp, struct componentname *tcnp,
    cred_t *cr)
{
	dmu_tx_t	*tx;
	zfsvfs_t	*zfsvfs;
	zilog_t		*zilog;
	znode_t		*tdzp, *sdzp, *tzp, *szp;
	const char	*snm = scnp->cn_nameptr;
	const char	*tnm = tcnp->cn_nameptr;
	int		error;

	tdzp = VTOZ(tdvp);
	sdzp = VTOZ(sdvp);
	zfsvfs = tdzp->z_zfsvfs;

	if ((error = zfs_enter_verify_zp(zfsvfs, tdzp, FTAG)) != 0)
		return (error);
	if ((error = zfs_verify_zp(sdzp)) != 0) {
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}
	zilog = zfsvfs->z_log;

	if (zfsvfs->z_utf8 && u8_validate(tnm,
	    strlen(tnm), NULL, U8_VALIDATE_ENTIRE, &error) < 0) {
		error = SET_ERROR(EILSEQ);
		goto out;
	}

	/* If source and target are the same file, there is nothing to do. */
	if ((*svpp) == (*tvpp)) {
		error = 0;
		goto out;
	}

	if (((*svpp)->v_type == VDIR && (*svpp)->v_mountedhere != NULL) ||
	    ((*tvpp) != NULL && (*tvpp)->v_type == VDIR &&
	    (*tvpp)->v_mountedhere != NULL)) {
		error = SET_ERROR(EXDEV);
		goto out;
	}

	szp = VTOZ(*svpp);
	if ((error = zfs_verify_zp(szp)) != 0) {
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}
	tzp = *tvpp == NULL ? NULL : VTOZ(*tvpp);
	if (tzp != NULL) {
		if ((error = zfs_verify_zp(tzp)) != 0) {
			zfs_exit(zfsvfs, FTAG);
			return (error);
		}
	}

	/*
	 * This is to prevent the creation of links into attribute space
	 * by renaming a linked file into/outof an attribute directory.
	 * See the comment in zfs_link() for why this is considered bad.
	 */
	if ((tdzp->z_pflags & ZFS_XATTR) != (sdzp->z_pflags & ZFS_XATTR)) {
		error = SET_ERROR(EINVAL);
		goto out;
	}

	/*
	 * If we are using project inheritance, means if the directory has
	 * ZFS_PROJINHERIT set, then its descendant directories will inherit
	 * not only the project ID, but also the ZFS_PROJINHERIT flag. Under
	 * such case, we only allow renames into our tree when the project
	 * IDs are the same.
	 */
	if (tdzp->z_pflags & ZFS_PROJINHERIT &&
	    tdzp->z_projid != szp->z_projid) {
		error = SET_ERROR(EXDEV);
		goto out;
	}

	/*
	 * Must have write access at the source to remove the old entry
	 * and write access at the target to create the new entry.
	 * Note that if target and source are the same, this can be
	 * done in a single check.
	 */
	if ((error = zfs_zaccess_rename(sdzp, szp, tdzp, tzp, cr, NULL)))
		goto out;

	if ((*svpp)->v_type == VDIR) {
		/*
		 * Avoid ".", "..", and aliases of "." for obvious reasons.
		 */
		if ((scnp->cn_namelen == 1 && scnp->cn_nameptr[0] == '.') ||
		    sdzp == szp ||
		    (scnp->cn_flags | tcnp->cn_flags) & ISDOTDOT) {
			error = EINVAL;
			goto out;
		}

		/*
		 * Check to make sure rename is valid.
		 * Can't do a move like this: /usr/a/b to /usr/a/b/c/d
		 */
		if ((error = zfs_rename_check(szp, sdzp, tdzp)))
			goto out;
	}

	/*
	 * Does target exist?
	 */
	if (tzp) {
		/*
		 * Source and target must be the same type.
		 */
		if ((*svpp)->v_type == VDIR) {
			if ((*tvpp)->v_type != VDIR) {
				error = SET_ERROR(ENOTDIR);
				goto out;
			} else {
				cache_purge(tdvp);
				if (sdvp != tdvp)
					cache_purge(sdvp);
			}
		} else {
			if ((*tvpp)->v_type == VDIR) {
				error = SET_ERROR(EISDIR);
				goto out;
			}
		}
	}

	vn_seqc_write_begin(*svpp);
	vn_seqc_write_begin(sdvp);
	if (*tvpp != NULL)
		vn_seqc_write_begin(*tvpp);
	if (tdvp != *tvpp)
		vn_seqc_write_begin(tdvp);

	vnevent_rename_src(*svpp, sdvp, scnp->cn_nameptr, ct);
	if (tzp)
		vnevent_rename_dest(*tvpp, tdvp, tnm, ct);

	/*
	 * notify the target directory if it is not the same
	 * as source directory.
	 */
	if (tdvp != sdvp) {
		vnevent_rename_dest_dir(tdvp, ct);
	}

	tx = dmu_tx_create(zfsvfs->z_os);
	dmu_tx_hold_sa(tx, szp->z_sa_hdl, B_FALSE);
	dmu_tx_hold_sa(tx, sdzp->z_sa_hdl, B_FALSE);
	dmu_tx_hold_zap(tx, sdzp->z_id, FALSE, snm);
	dmu_tx_hold_zap(tx, tdzp->z_id, TRUE, tnm);
	if (sdzp != tdzp) {
		dmu_tx_hold_sa(tx, tdzp->z_sa_hdl, B_FALSE);
		zfs_sa_upgrade_txholds(tx, tdzp);
	}
	if (tzp) {
		dmu_tx_hold_sa(tx, tzp->z_sa_hdl, B_FALSE);
		zfs_sa_upgrade_txholds(tx, tzp);
	}

	zfs_sa_upgrade_txholds(tx, szp);
	dmu_tx_hold_zap(tx, zfsvfs->z_unlinkedobj, FALSE, NULL);
	error = dmu_tx_assign(tx, DMU_TX_WAIT);
	if (error) {
		dmu_tx_abort(tx);
		goto out_seq;
	}

	if (tzp)	/* Attempt to remove the existing target */
		error = zfs_link_destroy(tdzp, tnm, tzp, tx, 0, NULL);

	if (error == 0) {
		error = zfs_link_create(tdzp, tnm, szp, tx, ZRENAMING);
		if (error == 0) {
			szp->z_pflags |= ZFS_AV_MODIFIED;

			error = sa_update(szp->z_sa_hdl, SA_ZPL_FLAGS(zfsvfs),
			    (void *)&szp->z_pflags, sizeof (uint64_t), tx);
			ASSERT0(error);

			error = zfs_link_destroy(sdzp, snm, szp, tx, ZRENAMING,
			    NULL);
			if (error == 0) {
				zfs_log_rename(zilog, tx, TX_RENAME, sdzp,
				    snm, tdzp, tnm, szp);
			} else {
				/*
				 * At this point, we have successfully created
				 * the target name, but have failed to remove
				 * the source name.  Since the create was done
				 * with the ZRENAMING flag, there are
				 * complications; for one, the link count is
				 * wrong.  The easiest way to deal with this
				 * is to remove the newly created target, and
				 * return the original error.  This must
				 * succeed; fortunately, it is very unlikely to
				 * fail, since we just created it.
				 */
				VERIFY0(zfs_link_destroy(tdzp, tnm, szp, tx,
				    ZRENAMING, NULL));
			}
		}
		if (error == 0 && zfsvfs->z_use_namecache) {
			cache_vop_rename(sdvp, *svpp, tdvp, *tvpp, scnp, tcnp);
		}
	}

	dmu_tx_commit(tx);

out_seq:
	vn_seqc_write_end(*svpp);
	vn_seqc_write_end(sdvp);
	if (*tvpp != NULL)
		vn_seqc_write_end(*tvpp);
	if (tdvp != *tvpp)
		vn_seqc_write_end(tdvp);

out:
	if (error == 0 && zfsvfs->z_os->os_sync == ZFS_SYNC_ALWAYS)
		error = zil_commit(zilog, 0);
	zfs_exit(zfsvfs, FTAG);

	return (error);
}


/*
 * Insert the indicated symbolic reference entry into the directory.
 *
 *	IN:	dvp	- Directory to contain new symbolic link.
 *		link	- Name for new symlink entry.
 *		vap	- Attributes of new entry.
 *		cr	- credentials of caller.
 *		ct	- caller context
 *		flags	- case flags
 *		mnt_ns	- Unused on FreeBSD
 *
 *	RETURN:	0 on success, error code on failure.
 *
 * Timestamps:
 *	dvp - ctime|mtime updated
 */
int
zfs_symlink(znode_t *dzp, const char *name, vattr_t *vap,
    const char *link, znode_t **zpp, cred_t *cr, int flags, zidmap_t *mnt_ns)
{
	(void) flags;
	znode_t		*zp;
	dmu_tx_t	*tx;
	zfsvfs_t	*zfsvfs = dzp->z_zfsvfs;
	zilog_t		*zilog;
	uint64_t	len = strlen(link);
	int		error;
	zfs_acl_ids_t	acl_ids;
	boolean_t	fuid_dirtied;
	uint64_t	txtype = TX_SYMLINK;

	ASSERT3S(vap->va_type, ==, VLNK);

	if (is_nametoolong(zfsvfs, name))
		return (SET_ERROR(ENAMETOOLONG));

	if ((error = zfs_enter_verify_zp(zfsvfs, dzp, FTAG)) != 0)
		return (error);
	zilog = zfsvfs->z_log;

	if (zfsvfs->z_utf8 && u8_validate(name, strlen(name),
	    NULL, U8_VALIDATE_ENTIRE, &error) < 0) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EILSEQ));
	}

	if (len > MAXPATHLEN) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(ENAMETOOLONG));
	}

	if ((error = zfs_acl_ids_create(dzp, 0,
	    vap, cr, NULL, &acl_ids, NULL)) != 0) {
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	/*
	 * Attempt to lock directory; fail if entry already exists.
	 */
	error = zfs_dirent_lookup(dzp, name, &zp, ZNEW);
	if (error) {
		zfs_acl_ids_free(&acl_ids);
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	if ((error = zfs_zaccess(dzp, ACE_ADD_FILE, 0, B_FALSE, cr, mnt_ns))) {
		zfs_acl_ids_free(&acl_ids);
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	if (zfs_acl_ids_overquota(zfsvfs, &acl_ids, ZFS_DEFAULT_PROJID)) {
		zfs_acl_ids_free(&acl_ids);
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EDQUOT));
	}

	getnewvnode_reserve();
	tx = dmu_tx_create(zfsvfs->z_os);
	fuid_dirtied = zfsvfs->z_fuid_dirty;
	dmu_tx_hold_write(tx, DMU_NEW_OBJECT, 0, MAX(1, len));
	dmu_tx_hold_zap(tx, dzp->z_id, TRUE, name);
	dmu_tx_hold_sa_create(tx, acl_ids.z_aclp->z_acl_bytes +
	    ZFS_SA_BASE_ATTR_SIZE + len);
	dmu_tx_hold_sa(tx, dzp->z_sa_hdl, B_FALSE);
	if (!zfsvfs->z_use_sa && acl_ids.z_aclp->z_acl_bytes > ZFS_ACE_SPACE) {
		dmu_tx_hold_write(tx, DMU_NEW_OBJECT, 0,
		    acl_ids.z_aclp->z_acl_bytes);
	}
	if (fuid_dirtied)
		zfs_fuid_txhold(zfsvfs, tx);
	error = dmu_tx_assign(tx, DMU_TX_WAIT);
	if (error) {
		zfs_acl_ids_free(&acl_ids);
		dmu_tx_abort(tx);
		getnewvnode_drop_reserve();
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	/*
	 * Create a new object for the symlink.
	 * for version 4 ZPL datasets the symlink will be an SA attribute
	 */
	zfs_mknode(dzp, vap, tx, cr, 0, &zp, &acl_ids);

	if (fuid_dirtied)
		zfs_fuid_sync(zfsvfs, tx);

	if (zp->z_is_sa)
		error = sa_update(zp->z_sa_hdl, SA_ZPL_SYMLINK(zfsvfs),
		    __DECONST(void *, link), len, tx);
	else
		zfs_sa_symlink(zp, __DECONST(char *, link), len, tx);

	zp->z_size = len;
	(void) sa_update(zp->z_sa_hdl, SA_ZPL_SIZE(zfsvfs),
	    &zp->z_size, sizeof (zp->z_size), tx);
	/*
	 * Insert the new object into the directory.
	 */
	error = zfs_link_create(dzp, name, zp, tx, ZNEW);
	if (error != 0) {
		zfs_znode_delete(zp, tx);
		VOP_UNLOCK(ZTOV(zp));
		zrele(zp);
	} else {
		zfs_log_symlink(zilog, tx, txtype, dzp, zp, name, link);
	}

	zfs_acl_ids_free(&acl_ids);

	dmu_tx_commit(tx);

	getnewvnode_drop_reserve();

	if (error == 0) {
		*zpp = zp;

		if (zfsvfs->z_os->os_sync == ZFS_SYNC_ALWAYS)
			error = zil_commit(zilog, 0);
	}

	zfs_exit(zfsvfs, FTAG);
	return (error);
}

/*
 * Return, in the buffer contained in the provided uio structure,
 * the symbolic path referred to by vp.
 *
 *	IN:	vp	- vnode of symbolic link.
 *		uio	- structure to contain the link path.
 *		cr	- credentials of caller.
 *		ct	- caller context
 *
 *	OUT:	uio	- structure containing the link path.
 *
 *	RETURN:	0 on success, error code on failure.
 *
 * Timestamps:
 *	vp - atime updated
 */
static int
zfs_readlink(vnode_t *vp, zfs_uio_t *uio, cred_t *cr, caller_context_t *ct)
{
	(void) cr, (void) ct;
	znode_t		*zp = VTOZ(vp);
	zfsvfs_t	*zfsvfs = zp->z_zfsvfs;
	int		error;

	if ((error = zfs_enter_verify_zp(zfsvfs, zp, FTAG)) != 0)
		return (error);

	if (zp->z_is_sa)
		error = sa_lookup_uio(zp->z_sa_hdl,
		    SA_ZPL_SYMLINK(zfsvfs), uio);
	else
		error = zfs_sa_readlink(zp, uio);

	ZFS_ACCESSTIME_STAMP(zfsvfs, zp);

	zfs_exit(zfsvfs, FTAG);
	return (error);
}

/*
 * Insert a new entry into directory tdvp referencing svp.
 *
 *	IN:	tdvp	- Directory to contain new entry.
 *		svp	- vnode of new entry.
 *		name	- name of new entry.
 *		cr	- credentials of caller.
 *
 *	RETURN:	0 on success, error code on failure.
 *
 * Timestamps:
 *	tdvp - ctime|mtime updated
 *	 svp - ctime updated
 */
int
zfs_link(znode_t *tdzp, znode_t *szp, const char *name, cred_t *cr,
    int flags)
{
	(void) flags;
	znode_t		*tzp;
	zfsvfs_t	*zfsvfs = tdzp->z_zfsvfs;
	zilog_t		*zilog;
	dmu_tx_t	*tx;
	int		error;
	uint64_t	parent;
	uid_t		owner;

	ASSERT3S(ZTOV(tdzp)->v_type, ==, VDIR);

	if (is_nametoolong(zfsvfs, name))
		return (SET_ERROR(ENAMETOOLONG));

	if ((error = zfs_enter_verify_zp(zfsvfs, tdzp, FTAG)) != 0)
		return (error);
	zilog = zfsvfs->z_log;

	/*
	 * POSIX dictates that we return EPERM here.
	 * Better choices include ENOTSUP or EISDIR.
	 */
	if (ZTOV(szp)->v_type == VDIR) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EPERM));
	}

	if ((error = zfs_verify_zp(szp)) != 0) {
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	/*
	 * If we are using project inheritance, means if the directory has
	 * ZFS_PROJINHERIT set, then its descendant directories will inherit
	 * not only the project ID, but also the ZFS_PROJINHERIT flag. Under
	 * such case, we only allow hard link creation in our tree when the
	 * project IDs are the same.
	 */
	if (tdzp->z_pflags & ZFS_PROJINHERIT &&
	    tdzp->z_projid != szp->z_projid) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EXDEV));
	}

	if (szp->z_pflags & (ZFS_APPENDONLY |
	    ZFS_IMMUTABLE | ZFS_READONLY)) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EPERM));
	}

	/* Prevent links to .zfs/shares files */

	if ((error = sa_lookup(szp->z_sa_hdl, SA_ZPL_PARENT(zfsvfs),
	    &parent, sizeof (uint64_t))) != 0) {
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}
	if (parent == zfsvfs->z_shares_dir) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EPERM));
	}

	if (zfsvfs->z_utf8 && u8_validate(name,
	    strlen(name), NULL, U8_VALIDATE_ENTIRE, &error) < 0) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EILSEQ));
	}

	/*
	 * We do not support links between attributes and non-attributes
	 * because of the potential security risk of creating links
	 * into "normal" file space in order to circumvent restrictions
	 * imposed in attribute space.
	 */
	if ((szp->z_pflags & ZFS_XATTR) != (tdzp->z_pflags & ZFS_XATTR)) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EINVAL));
	}


	owner = zfs_fuid_map_id(zfsvfs, szp->z_uid, cr, ZFS_OWNER);
	if (owner != crgetuid(cr) && secpolicy_basic_link(ZTOV(szp), cr) != 0) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EPERM));
	}

	if ((error = zfs_zaccess(tdzp, ACE_ADD_FILE, 0, B_FALSE, cr, NULL))) {
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	/*
	 * Attempt to lock directory; fail if entry already exists.
	 */
	error = zfs_dirent_lookup(tdzp, name, &tzp, ZNEW);
	if (error) {
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	tx = dmu_tx_create(zfsvfs->z_os);
	dmu_tx_hold_sa(tx, szp->z_sa_hdl, B_FALSE);
	dmu_tx_hold_zap(tx, tdzp->z_id, TRUE, name);
	zfs_sa_upgrade_txholds(tx, szp);
	zfs_sa_upgrade_txholds(tx, tdzp);
	error = dmu_tx_assign(tx, DMU_TX_WAIT);
	if (error) {
		dmu_tx_abort(tx);
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	error = zfs_link_create(tdzp, name, szp, tx, 0);

	if (error == 0) {
		uint64_t txtype = TX_LINK;
		zfs_log_link(zilog, tx, txtype, tdzp, szp, name);
	}

	dmu_tx_commit(tx);

	if (error == 0) {
		vnevent_link(ZTOV(szp), ct);
	}

	if (error == 0 && zfsvfs->z_os->os_sync == ZFS_SYNC_ALWAYS)
		error = zil_commit(zilog, 0);

	zfs_exit(zfsvfs, FTAG);
	return (error);
}

/*
 * Free or allocate space in a file.  Currently, this function only
 * supports the `F_FREESP' command.  However, this command is somewhat
 * misnamed, as its functionality includes the ability to allocate as
 * well as free space.
 *
 *	IN:	ip	- inode of file to free data in.
 *		cmd	- action to take (only F_FREESP supported).
 *		bfp	- section of file to free/alloc.
 *		flag	- current file open mode flags.
 *		offset	- current file offset.
 *		cr	- credentials of caller.
 *
 *	RETURN:	0 on success, error code on failure.
 *
 * Timestamps:
 *	ip - ctime|mtime updated
 */
int
zfs_space(znode_t *zp, int cmd, flock64_t *bfp, int flag,
    offset_t offset, cred_t *cr)
{
	(void) offset;
	zfsvfs_t	*zfsvfs = ZTOZSB(zp);
	uint64_t	off, len;
	int		error;

	if ((error = zfs_enter_verify_zp(zfsvfs, zp, FTAG)) != 0)
		return (error);

	if (cmd != F_FREESP) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EINVAL));
	}

	/*
	 * Callers might not be able to detect properly that we are read-only,
	 * so check it explicitly here.
	 */
	if (zfs_is_readonly(zfsvfs)) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EROFS));
	}

	if (bfp->l_len < 0) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EINVAL));
	}

	/*
	 * Permissions aren't checked on Solaris because on this OS
	 * zfs_space() can only be called with an opened file handle.
	 * On Linux we can get here through truncate_range() which
	 * operates directly on inodes, so we need to check access rights.
	 */
	if ((error = zfs_zaccess(zp, ACE_WRITE_DATA, 0, B_FALSE, cr, NULL))) {
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	off = bfp->l_start;
	len = bfp->l_len; /* 0 means from off to end of file */

	error = zfs_freesp(zp, off, len, flag, TRUE);

	zfs_exit(zfsvfs, FTAG);
	return (error);
}

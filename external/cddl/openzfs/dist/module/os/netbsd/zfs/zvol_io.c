// SPDX-License-Identifier: CDDL-1.0
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
 * Copyright (c) 2006-2010 Pawel Jakub Dawidek <pjd@FreeBSD.org>
 * All rights reserved.
 * Portions Copyright 2010 Robert Milkowski
 * Copyright 2011 Nexenta Systems, Inc. All rights reserved.
 * Copyright (c) 2012, 2017 by Delphix. All rights reserved.
 * Copyright (c) 2013, Joyent, Inc. All rights reserved.
 * Copyright (c) 2014 Integros [integros.com]
 * Copyright (c) 2024, 2025, Klara, Inc.
 * Portions Copyright 2011 Martin Matuska <mm@FreeBSD.org>
 *
 * Native block and raw volume I/O. As in osnet, strategy calls complete
 * synchronously; all suspend-lock acquisition and release stays on one LWP.
 */

#include <sys/zfs_context.h>
#include <sys/dmu_objset.h>
#include <sys/dmu_tx.h>
#include <sys/dkio.h>
#include <sys/disklabel.h>
#include <sys/spa_impl.h>
#include <sys/zil_impl.h>
#include <sys/zvol_impl_os.h>
#include <sys/zvol_os.h>

static unsigned int
zvol_openbit(int fmt)
{
	return (fmt == S_IFBLK ? 1 : 2);
}

int
zvol_open(dev_t dev, int flags, int fmt, lwp_t *l)
{
	zvol_state_t *zv;
	boolean_t drop_namespace;
	int error;

retry:
	zv = zvol_os_hold(dev, B_FALSE);
	if (zv == NULL)
		return (SET_ERROR(ENXIO));
	error = 0;
	if (zv->zv_open_count == 0) {
		/*
		 * A pool may open a local zvol as a vdev. Follow OpenZFS's
		 * try/retry ordering, rather than blocking for the namespace
		 * while holding locks needed by pool minor management.
		 */
		drop_namespace = !spa_namespace_held();
		if (drop_namespace && !spa_namespace_tryenter(FTAG)) {
			zvol_os_rele(zv);
			kpause("zvolopen", false, 1, NULL);
			goto retry;
		}
		error = zvol_first_open(zv, !(flags & FWRITE));
		if (drop_namespace)
			spa_namespace_exit(FTAG);
		if (error != 0)
			goto out;
	}
	if ((flags & FWRITE) && (zv->zv_flags & ZVOL_RDONLY))
		error = EROFS;
	else if ((zv->zv_flags & ZVOL_EXCL) ||
	    ((flags & O_EXCL) && zv->zv_open_count != 0))
		error = EBUSY;
	else {
		if (flags & O_EXCL)
			zv->zv_flags |= ZVOL_EXCL;
		unsigned int bit = zvol_openbit(fmt);
		/* specfs calls close once per type, even after several opens. */
		if (!(zv->zv_zso->zos_openmask & bit)) {
			zv->zv_zso->zos_openmask |= bit;
			zv->zv_open_count++;
		}
	}
	if (zv->zv_open_count == 0)
		zvol_last_close(zv);
out:
	zvol_os_rele(zv);
	return (error);
}

int
zvol_close(dev_t dev, int flags, int fmt, lwp_t *l)
{
	zvol_state_t *zv = zvol_os_hold(dev, B_TRUE);
	unsigned int bit = zvol_openbit(fmt);

	if (zv == NULL)
		return (SET_ERROR(ENXIO));
	ASSERT(zv->zv_zso->zos_openmask & bit);
	ASSERT3U(zv->zv_open_count, >, 0);
	zv->zv_zso->zos_openmask &= ~bit;
	zv->zv_open_count--;
	zv->zv_flags &= ~ZVOL_EXCL;
	if (zv->zv_open_count == 0) {
		zvol_last_close(zv);
		cv_broadcast(&zv->zv_removing_cv);
	}
	zvol_os_rele(zv);
	return (0);
}

static void
zvol_ensure_zilog(zvol_state_t *zv)
{
	ASSERT(MUTEX_HELD(&zv->zv_state_lock));
	ASSERT(RW_READ_HELD(&zv->zv_suspend_lock));

	/* Serialize lazy creation before releasing the state lock for I/O. */
	if (zv->zv_zilog == NULL) {
		zv->zv_zilog = zil_open(zv->zv_objset, zvol_get_data,
		    &zv->zv_kstat.dk_zil_sums);
		zv->zv_flags |= ZVOL_WRITTEN_TO;
		VERIFY0(zv->zv_zilog->zl_header->zh_flags & ZIL_REPLAY_NEEDED);
	}
}

static int
zvol_rw(dev_t dev, struct uio *native, int flags)
{
	zvol_state_t *zv = zvol_os_hold(dev, B_FALSE);
	struct zvol_state_os *zso;
	zfs_locked_range_t *lr;
	zfs_uio_t uio;
	uint64_t volsize;
	size_t before = native->uio_resid;
	boolean_t doread = native->uio_rw == UIO_READ;
	boolean_t commit;
	int error = 0;

	if (zv == NULL)
		return (SET_ERROR(ENXIO));
	if (zv->zv_open_count == 0 || zv->zv_objset == NULL) {
		error = ENXIO;
		goto out;
	}
	if (before == 0)
		goto out;
	volsize = zv->zv_volsize;
	if (native->uio_offset < 0 || native->uio_offset > volsize) {
		error = EIO;
		goto out;
	}
	if (!doread && (zv->zv_flags & ZVOL_RDONLY)) {
		error = EROFS;
		goto out;
	}
	if (native->uio_offset == volsize) {
		error = doread ? 0 : ENOSPC;
		goto out;
	}
	commit = !doread && ((flags & IO_SYNC) ||
	    zv->zv_objset->os_sync == ZFS_SYNC_ALWAYS);
	if (!doread)
		zvol_ensure_zilog(zv);
	zso = zv->zv_zso;
	mutex_exit(&zv->zv_state_lock);

	mutex_enter(&zso->zos_disk_lock);
	disk_busy(&zso->zos_disk);
	mutex_exit(&zso->zos_disk_lock);
	zfs_uio_init(&uio, native);
	lr = zfs_rangelock_enter(&zv->zv_rangelock, native->uio_offset,
	    MIN(before, volsize - native->uio_offset),
	    doread ? RL_READER : RL_WRITER);
	while (native->uio_resid != 0 && native->uio_offset < volsize) {
		uint64_t off = native->uio_offset;
		size_t bytes = MIN(native->uio_resid, DMU_MAX_ACCESS >> 1);

		bytes = MIN(bytes, volsize - off);
		if (doread) {
			error = dmu_read_uio_dnode(zv->zv_dn, &uio, bytes,
			    DMU_READ_PREFETCH);
		} else {
			dmu_tx_t *tx = dmu_tx_create(zv->zv_objset);
			dmu_tx_hold_write_by_dnode(tx, zv->zv_dn, off, bytes);
			error = dmu_tx_assign(tx, DMU_TX_WAIT);
			if (error != 0) {
				dmu_tx_abort(tx);
				break;
			}
			error = dmu_write_uio_dnode(zv->zv_dn, &uio, bytes,
			    tx, DMU_READ_PREFETCH);
			/* Also log any bytes completed before a user-copy fault. */
			if (native->uio_offset != off)
				zvol_log_write(zv, tx, off,
				    native->uio_offset - off, commit);
			dmu_tx_commit(tx);
		}
		if (error != 0)
			break;
	}
	zfs_rangelock_exit(lr);
	if (commit && before != native->uio_resid) {
		int commit_error = zil_commit(zv->zv_zilog, ZVOL_OBJ);
		if (error == 0)
			error = commit_error;
	}
	if (error == ECKSUM)
		error = EIO;
	if (doread)
		dataset_kstats_update_read_kstats(&zv->zv_kstat,
		    before - native->uio_resid);
	else
		dataset_kstats_update_write_kstats(&zv->zv_kstat,
		    before - native->uio_resid);
	mutex_enter(&zso->zos_disk_lock);
	disk_unbusy(&zso->zos_disk, before - native->uio_resid, doread);
	mutex_exit(&zso->zos_disk_lock);
	mutex_enter(&zv->zv_state_lock);
out:
	zvol_os_rele(zv);
	return (error);
}

int
zvol_read(dev_t dev, struct uio *uio, int flags)
{
	return (zvol_rw(dev, uio, flags));
}

int
zvol_write(dev_t dev, struct uio *uio, int flags)
{
	return (zvol_rw(dev, uio, flags));
}

void
zvol_strategy(struct buf *bp)
{
	struct iovec iov = { .iov_base = bp->b_data, .iov_len = bp->b_bcount };
	struct uio uio = {
		.uio_iov = &iov,
		.uio_iovcnt = 1,
		.uio_resid = bp->b_bcount,
		.uio_rw = (bp->b_flags & B_READ) ? UIO_READ : UIO_WRITE,
	};

	/* b_blkno is in DEV_BSIZE units, independent of reported geometry. */
	if (bp->b_blkno < 0 || bp->b_blkno > INT64_MAX >> DEV_BSHIFT) {
		bp->b_error = EINVAL;
		bp->b_resid = bp->b_bcount;
	} else {
		uio.uio_offset = (uint64_t)bp->b_blkno << DEV_BSHIFT;
		UIO_SETUP_SYSSPACE(&uio);
		bp->b_error = zvol_rw(bp->b_dev, &uio,
		    (bp->b_flags & B_ASYNC) ? 0 : IO_SYNC);
		bp->b_resid = uio.uio_resid;
	}
	biodone(bp);
}

int
zvol_ioctl(dev_t dev, u_long cmd, void *data, int flags, lwp_t *l)
{
	zvol_state_t *zv = zvol_os_hold(dev, B_FALSE);
	struct zvol_state_os *zso;
	struct disk_geom *dg;
	int error;

	if (zv == NULL)
		return (SET_ERROR(ENXIO));
	if (zv->zv_open_count == 0 || zv->zv_objset == NULL) {
		error = ENXIO;
		goto out;
	}
	zso = zv->zv_zso;
	mutex_enter(&zso->zos_disk_lock);
	error = disk_ioctl(&zso->zos_disk, NODEV, cmd, data, flags, l);
	mutex_exit(&zso->zos_disk_lock);
	if (error != EPASSTHROUGH)
		goto out;
	error = 0;
	switch (cmd) {
	case DIOCCACHESYNC: {
		zilog_t *zilog = zv->zv_zilog;
		mutex_exit(&zv->zv_state_lock);
		if (zilog != NULL)
			error = zil_commit(zilog, ZVOL_OBJ);
		mutex_enter(&zv->zv_state_lock);
		break;
	}
	case DIOCGWEDGEINFO: {
		struct dkwedge_info *dkw = data;
		memset(dkw, 0, sizeof (*dkw));
		strlcpy(dkw->dkw_devname, zv->zv_name, sizeof (dkw->dkw_devname));
		/* As in osnet, force getdisksize to fall back to DIOCGPARTINFO. */
		strlcpy(dkw->dkw_parent, "ZFS", sizeof (dkw->dkw_parent));
		mutex_enter(&zso->zos_disk_lock);
		dg = &zso->zos_disk.dk_geom;
		dkw->dkw_size = dg->dg_secperunit;
		mutex_exit(&zso->zos_disk_lock);
		strlcpy(dkw->dkw_ptype, DKW_PTYPE_FFS, sizeof (dkw->dkw_ptype));
		break;
	}
	case DIOCGPARTINFO: {
		struct partinfo *pi = data;
		memset(pi, 0, sizeof (*pi));
		mutex_enter(&zso->zos_disk_lock);
		dg = &zso->zos_disk.dk_geom;
		pi->pi_secsize = dg->dg_secsize;
		pi->pi_size = dg->dg_secperunit;
		mutex_exit(&zso->zos_disk_lock);
		pi->pi_fstype = FS_OTHER;
		pi->pi_bsize = MAX(BLKDEV_IOSIZE, pi->pi_secsize);
		break;
	}
	default:
		error = ENOTTY;
		break;
	}
out:
	zvol_os_rele(zv);
	return (error);
}

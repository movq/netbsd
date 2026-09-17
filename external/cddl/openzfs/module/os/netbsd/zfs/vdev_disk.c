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
 * Copyright (c) 2012, 2015 by Delphix. All rights reserved.
 * Copyright 2013 Nexenta Systems, Inc. All rights reserved.
 * Copyright (c) 2013 Joyent, Inc. All rights reserved.
 *
 * Adapted from the NetBSD osnet disk vdev. Keep native buffer I/O and
 * workqueue cache flushes, with ABD ownership handled in io_done.
 */

#include <sys/zfs_context.h>
#include <sys/abd.h>
#include <sys/spa.h>
#include <sys/vdev_impl.h>
#include <sys/zio.h>
#include <sys/disk.h>
#include <sys/dkio.h>
#include <sys/workqueue.h>
#include <miscfs/specfs/specdev.h>

typedef struct vdev_disk {
	vnode_t		*vd_vp;
	struct workqueue *vd_wq;
	int		vd_maxphys;
	int		vd_mode;
} vdev_disk_t;

static void
vdev_disk_flush(struct work *work, void *cookie)
{
	buf_t *bp = (buf_t *)work;
	vdev_disk_t *dvd = cookie;
	zio_t *zio = bp->b_private;
	int cmd = 1;

	zio->io_error = VOP_IOCTL(dvd->vd_vp, DIOCCACHESYNC, &cmd,
	    dvd->vd_mode, kcred);
	if (zio->io_error == ENOTTY || zio->io_error == EOPNOTSUPP) {
		zio->io_vd->vdev_nowritecache = B_TRUE;
		zio->io_error = SET_ERROR(ENOTSUP);
	}
	putiobuf(bp);
	zio_interrupt(zio);
}

static int
vdev_disk_open(vdev_t *vd, uint64_t *psize, uint64_t *max_psize,
    uint64_t *ashift, uint64_t *pashift, cred_t *cr)
{
	vdev_disk_t *dvd;
	vnode_t *vp;
	struct pathbuf *pb;
	struct disk *pdk = NULL;
	struct dkwedge_info dkw;
	struct disk_sectoralign dsa;
	uint64_t numsecs;
	unsigned secsize;
	int error;

	/* The old NetBSD disk interface has no discard support. */
	vd->vdev_has_trim = B_FALSE;
	vd->vdev_has_securetrim = B_FALSE;

	if (vd->vdev_path == NULL || vd->vdev_path[0] != '/') {
		vd->vdev_stat.vs_aux = VDEV_AUX_BAD_LABEL;
		return (SET_ERROR(EINVAL));
	}

	if (vd->vdev_tsd != NULL) {
		ASSERT(vd->vdev_reopening);
		dvd = vd->vdev_tsd;
		vp = dvd->vd_vp;
		ASSERT3P(vp, !=, NULL);
		goto skip_open;
	}

	dvd = vd->vdev_tsd = kmem_zalloc(sizeof (*dvd), KM_SLEEP);
	dvd->vd_mode = FREAD;
	if (spa_writeable(vd->vdev_spa))
		dvd->vd_mode |= FWRITE;

	/* As in osnet, native vn_open uses the calling LWP's credentials. */
	(void) cr;
	pb = pathbuf_create(vd->vdev_path);
	error = vn_open(NULL, pb, 0, dvd->vd_mode, 0, &vp, NULL, NULL);
	pathbuf_destroy(pb);
	if (error != 0)
		goto fail;
	VOP_UNLOCK(vp);
	if (vp->v_type != VBLK) {
		(void) vn_close(vp, dvd->vd_mode, kcred);
		error = EINVAL;
		goto fail;
	}
	dvd->vd_vp = vp;

	if (getdiskinfo(vp, &dkw) == 0)
		pdk = disk_find(dkw.dkw_devname);
	struct buf buf = {
		.b_dev = vp->v_specnode->sn_rdev,
		.b_bcount = MAXPHYS,
	};
	if (pdk != NULL && pdk->dk_driver != NULL &&
	    pdk->dk_driver->d_minphys != NULL)
		(*pdk->dk_driver->d_minphys)(&buf);
	dvd->vd_maxphys = buf.b_bcount;
	if (dvd->vd_maxphys <= 0) {
		error = EINVAL;
		goto fail;
	}

	error = workqueue_create(&dvd->vd_wq, "vdevsync",
	    vdev_disk_flush, dvd, PRI_NONE, IPL_NONE, WQ_MPSAFE);
	if (error != 0)
		goto fail;

skip_open:
	error = getdisksize(vp, &numsecs, &secsize);
	if (error != 0)
		goto fail;
	if (secsize == 0 || !ISP2(secsize) ||
	    numsecs > UINT64_MAX / secsize) {
		error = EINVAL;
		goto fail;
	}

	*max_psize = *psize = numsecs * secsize;
	*ashift = highbit64(MAX(secsize, SPA_MINBLOCKSIZE)) - 1;
	*pashift = *ashift;
	if (VOP_IOCTL(vp, DIOCGSECTORALIGN, &dsa, FREAD, kcred) == 0 &&
	    dsa.dsa_alignment != 0) {
		*pashift = highbit64(MAX((uint64_t)dsa.dsa_alignment * secsize,
		    SPA_MINBLOCKSIZE)) - 1;
		if (dsa.dsa_firstaligned % dsa.dsa_alignment != 0)
			printf("ZFS WARNING: vdev %s: sectors are misaligned "
			    "(alignment=%" PRIu32 ", firstaligned=%" PRIu32
			    ")\n", vd->vdev_path, dsa.dsa_alignment,
			    dsa.dsa_firstaligned);
	}

	vd->vdev_wholedisk = 0;
	if (getdiskinfo(vp, &dkw) == 0 &&
	    dkw.dkw_offset == 0 && dkw.dkw_size == numsecs)
		vd->vdev_wholedisk = 1;
	vd->vdev_nowritecache = B_FALSE;
	return (0);

fail:
	/* The vdev layer calls close to release a partially opened device. */
	vd->vdev_stat.vs_aux = VDEV_AUX_OPEN_FAILED;
	return (SET_ERROR(error));
}

static void
vdev_disk_close(vdev_t *vd)
{
	vdev_disk_t *dvd = vd->vdev_tsd;

	if (vd->vdev_reopening || dvd == NULL)
		return;

	/* Drain flush callbacks before releasing the vnode they use. */
	if (dvd->vd_wq != NULL)
		workqueue_destroy(dvd->vd_wq);
	if (dvd->vd_vp != NULL)
		(void) vn_close(dvd->vd_vp, dvd->vd_mode, kcred);
	kmem_free(dvd, sizeof (*dvd));
	vd->vdev_tsd = NULL;
	vd->vdev_delayed_close = B_FALSE;
}

static void
vdev_disk_io_intr(buf_t *bp)
{
	zio_t *zio = bp->b_private;

	zio->io_error = bp->b_error != 0 || bp->b_resid != 0 ?
	    SET_ERROR(EIO) : 0;
	/*
	 * ABD copies and buffer disposal may sleep. Retain the buffer until
	 * io_done runs on a ZIO taskq, outside the device interrupt.
	 */
	zio_delay_interrupt(zio);
}

static void
vdev_disk_io_start(zio_t *zio)
{
	vdev_t *vd = zio->io_vd;
	vdev_disk_t *dvd = vd->vdev_tsd;
	vnode_t *vp;
	buf_t *bp;

	if (dvd == NULL || dvd->vd_vp == NULL) {
		zio->io_error = SET_ERROR(ENXIO);
		zio_interrupt(zio);
		return;
	}
	vp = dvd->vd_vp;

	if (zio->io_type == ZIO_TYPE_FLUSH) {
		if (!vdev_readable(vd))
			zio->io_error = SET_ERROR(ENXIO);
		else if (zfs_nocacheflush)
			zio->io_error = 0;
		else if (vd->vdev_nowritecache)
			zio->io_error = SET_ERROR(ENOTSUP);
		else {
			bp = getiobuf(vp, true);
			bp->b_private = zio;
			workqueue_enqueue(dvd->vd_wq, &bp->b_work, NULL);
			return;
		}
		zio_interrupt(zio);
		return;
	}

	if (zio->io_type == ZIO_TYPE_TRIM) {
		zio->io_error = SET_ERROR(ENOTSUP);
		zio_interrupt(zio);
		return;
	}

	ASSERT(zio->io_type == ZIO_TYPE_READ || zio->io_type == ZIO_TYPE_WRITE);
	ASSERT3U(dvd->vd_maxphys, >, 0);
	zio->io_target_timestamp = zio_handle_io_delay(zio);

	bp = getiobuf(vp, true);
	bp->b_flags = zio->io_type == ZIO_TYPE_READ ? B_READ : B_WRITE;
	bp->b_cflags = BC_BUSY | BC_NOCACHE;
	bp->b_dev = vp->v_specnode->sn_rdev;
	bp->b_data = zio->io_type == ZIO_TYPE_READ ?
	    abd_borrow_buf(zio->io_abd, zio->io_size) :
	    abd_borrow_buf_copy(zio->io_abd, zio->io_size);
	bp->b_blkno = btodb(zio->io_offset);
	bp->b_bcount = bp->b_resid = zio->io_size;
	bp->b_iodone = vdev_disk_io_intr;
	bp->b_private = zio;
	zio->io_bio = bp;

	if (zio->io_type == ZIO_TYPE_WRITE) {
		mutex_enter(vp->v_interlock);
		vp->v_numoutput++;
		mutex_exit(vp->v_interlock);
	}

	if (zio->io_size <= dvd->vd_maxphys) {
		(void) VOP_STRATEGY(vp, bp);
	} else {
		size_t resid = zio->io_size;
		size_t off = 0;

		while (resid != 0) {
			size_t size = MIN(resid, dvd->vd_maxphys);
			buf_t *nbp = getiobuf(vp, true);

			nbp->b_blkno = btodb(zio->io_offset + off);
			nestiobuf_setup(bp, nbp, off, size);
			(void) VOP_STRATEGY(vp, nbp);
			resid -= size;
			off += size;
		}
	}
}

static void
vdev_disk_io_done(zio_t *zio)
{
	buf_t *bp = zio->io_bio;

	/* Flushes, unsupported operations, or I/O rejected before submission. */
	if (bp == NULL)
		return;
	if (zio->io_type == ZIO_TYPE_READ)
		abd_return_buf_copy(zio->io_abd, bp->b_data, zio->io_size);
	else
		abd_return_buf(zio->io_abd, bp->b_data, zio->io_size);
	putiobuf(bp);
	zio->io_bio = NULL;
}

vdev_ops_t vdev_disk_ops = {
	.vdev_op_open = vdev_disk_open,
	.vdev_op_close = vdev_disk_close,
	.vdev_op_psize_to_asize = vdev_default_asize,
	.vdev_op_asize_to_psize = vdev_default_psize,
	.vdev_op_min_asize = vdev_default_min_asize,
	.vdev_op_io_start = vdev_disk_io_start,
	.vdev_op_io_done = vdev_disk_io_done,
	.vdev_op_xlate = vdev_default_xlate,
	.vdev_op_type = VDEV_TYPE_DISK,
	.vdev_op_leaf = B_TRUE,
};

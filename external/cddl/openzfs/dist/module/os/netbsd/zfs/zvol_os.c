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
 * NetBSD disk and minor-node integration, adapted from osnet and the
 * OpenZFS FreeBSD volume lifecycle.
 */

#include <sys/zfs_context.h>
#include <sys/dmu_objset.h>
#include <sys/dmu_tx.h>
#include <sys/dnode.h>
#include <sys/dsl_prop.h>
#include <sys/spa_impl.h>
#include <sys/stat.h>
#include <sys/zap.h>
#include <sys/zil.h>
#include <sys/zvol_impl_os.h>
#include <sys/zvol_os.h>
#include <sys/zfs_ioctl_os.h>
#include <sys/zfs_ioctl.h>

static kmutex_t zvol_dev_lock;
static kcondvar_t zvol_dev_cv;
static list_t zvol_devices;
static vmem_t *zvol_minor_arena;
static volatile unsigned int zvol_minors;

static const struct dkdriver zvol_dkdriver = {
	.d_strategy = zvol_strategy,
	.d_minphys = minphys,
};

/*
 * The device map has its own references because a lookup may be waiting for
 * zv_suspend_lock while common-code removal unpublishes a volume. Never
 * acquire a ZFS lock with zvol_dev_lock held.
 */
zvol_state_t *
zvol_os_hold(dev_t dev, boolean_t closing)
{
	struct zvol_state_os *zso;
	zvol_state_t *zv = NULL;

	mutex_enter(&zvol_dev_lock);
	for (zso = list_head(&zvol_devices); zso != NULL;
	    zso = list_next(&zvol_devices, zso)) {
		if (zso->zos_minor == minor(dev)) {
			zso->zos_refs++;
			zv = zso->zos_zv;
			break;
		}
	}
	mutex_exit(&zvol_dev_lock);
	if (zv == NULL)
		return (NULL);

	rw_enter(&zv->zv_suspend_lock, RW_READER);
	mutex_enter(&zv->zv_state_lock);
	if (!closing && (zv->zv_flags & ZVOL_REMOVING)) {
		zvol_os_rele(zv);
		return (NULL);
	}
	return (zv);
}

void
zvol_os_rele(zvol_state_t *zv)
{
	struct zvol_state_os *zso = zv->zv_zso;

	mutex_exit(&zv->zv_state_lock);
	rw_exit(&zv->zv_suspend_lock);
	mutex_enter(&zvol_dev_lock);
	ASSERT3U(zso->zos_refs, >, 0);
	if (--zso->zos_refs == 0)
		cv_broadcast(&zvol_dev_cv);
	mutex_exit(&zvol_dev_lock);
}

static dev_info_t
zvol_devinfo(void)
{
	dev_info_t di = { .di_cmajor = zfs_cmajor, .di_bmajor = zfs_bmajor };

	return (di);
}

static int
zvol_create_nodes(const char *name, minor_t m)
{
	dev_info_t di = zvol_devinfo();
	int error;

	error = ddi_create_minor_node(&di, __UNCONST(name), S_IFCHR, m,
	    DDI_PSEUDO, 0);
	if (error == 0) {
		error = ddi_create_minor_node(&di, __UNCONST(name), S_IFBLK, m,
		    DDI_PSEUDO, 0);
		if (error != 0)
			ddi_remove_minor_node(&di, __UNCONST(name));
	}
	return (error);
}

static int
zvol_alloc_minor(const char *name, vmem_addr_t *mp)
{
	char *path = kmem_asprintf("/dev/zvol/dsk/%s", name);
	vnode_t *vp;
	struct stat sb;
	int error;

	/* Preserve existing device numbers, as the old NetBSD port did. */
	error = namei_simple_kernel(path, NSM_NOFOLLOW_NOEMULROOT, &vp);
	kmem_strfree(path);
	if (error == 0) {
		vn_lock(vp, LK_SHARED | LK_RETRY);
		error = vn_stat(vp, &sb);
		VOP_UNLOCK(vp);
		vrele(vp);
		if (error == 0 && S_ISBLK(sb.st_mode) &&
		    major(sb.st_rdev) == zfs_bmajor &&
		    minor(sb.st_rdev) > 0 &&
		    minor(sb.st_rdev) <= ZFSDEV_MAX_MINOR) {
			*mp = minor(sb.st_rdev);
			if (vmem_xalloc_addr(zvol_minor_arena, *mp, 1,
			    VM_NOSLEEP) == 0)
				return (0);
		}
	}
	return (vmem_xalloc(zvol_minor_arena, 1, 1, 0, 0, 1,
	    ZFSDEV_MAX_MINOR, VM_NOSLEEP | VM_INSTANTFIT, mp));
}

static void
zvol_set_geometry(zvol_state_t *zv, uint64_t size)
{
	struct zvol_state_os *zso = zv->zv_zso;
	struct disk_geom *dg = &zso->zos_disk.dk_geom;

	mutex_enter(&zso->zos_disk_lock);
	memset(dg, 0, sizeof (*dg));
	dg->dg_secsize = DEV_BSIZE;
	if (zv->zv_objset != NULL) {
		spa_t *spa = dmu_objset_spa(zv->zv_objset);
		dg->dg_secsize = MAX(DEV_BSIZE, 1U << spa->spa_max_ashift);
	}
	dg->dg_secperunit = size / dg->dg_secsize;
	disk_set_info(NULL, &zso->zos_disk, "ZVOL");
	mutex_exit(&zso->zos_disk_lock);
}

static void
zvol_free_state(zvol_state_t *zv)
{
	struct zvol_state_os *zso = zv->zv_zso;

	if (zso != NULL) {
		disk_destroy(&zso->zos_disk);
		mutex_destroy(&zso->zos_disk_lock);
		vmem_xfree(zvol_minor_arena, zso->zos_minor, 1);
		kmem_free(zso, sizeof (*zso));
	}
	dataset_kstats_destroy(&zv->zv_kstat);
	zfs_rangelock_fini(&zv->zv_rangelock);
	rw_destroy(&zv->zv_suspend_lock);
	mutex_destroy(&zv->zv_state_lock);
	cv_destroy(&zv->zv_removing_cv);
	kmem_free(zv, sizeof (*zv));
}

int
zvol_os_create_minor(const char *name)
{
	objset_t *os;
	zvol_state_t *zv;
	struct zvol_state_os *zso;
	dmu_object_info_t *doi;
	vmem_addr_t m;
	uint64_t volsize, volmode, len;
	int error;

	if (zvol_inhibit_dev)
		return (0);
	zv = zvol_find_by_name_hash(name, zvol_name_hash(name), RW_NONE);
	if (zv != NULL) {
		mutex_exit(&zv->zv_state_lock);
		return (SET_ERROR(EEXIST));
	}
	error = dsl_prop_get_integer(name, "volmode", &volmode, NULL);
	if (error != 0)
		return (error);
	if (volmode == ZFS_VOLMODE_DEFAULT)
		volmode = zvol_volmode;
	if (volmode == ZFS_VOLMODE_NONE)
		return (0);

	error = dmu_objset_own(name, DMU_OST_ZVOL, B_TRUE, B_TRUE, FTAG, &os);
	if (error != 0)
		return (error);
	doi = kmem_alloc(sizeof (*doi), KM_SLEEP);
	error = dmu_object_info(os, ZVOL_OBJ, doi);
	if (error != 0)
		goto out;
	error = zap_lookup(os, ZVOL_ZAP_OBJ, "size", 8, 1, &volsize);
	if (error != 0)
		goto out;
	error = zvol_alloc_minor(name, &m);
	if (error != 0)
		goto out;

	zv = kmem_zalloc(sizeof (*zv), KM_SLEEP);
	zso = zv->zv_zso = kmem_zalloc(sizeof (*zso), KM_SLEEP);
	zso->zos_minor = m;
	zso->zos_zv = zv;
	strlcpy(zv->zv_name, name, sizeof (zv->zv_name));
	zv->zv_hash = zvol_name_hash(name);
	zv->zv_volmode = volmode;
	zv->zv_volsize = volsize;
	zv->zv_volblocksize = doi->doi_data_block_size;
	zv->zv_objset = os;
	mutex_init(&zv->zv_state_lock, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&zv->zv_removing_cv, NULL, CV_DEFAULT, NULL);
	rw_init(&zv->zv_suspend_lock, NULL, RW_DEFAULT, NULL);
	zfs_rangelock_init(&zv->zv_rangelock, NULL, NULL);
	mutex_init(&zso->zos_disk_lock, NULL, MUTEX_DEFAULT, NULL);
	disk_init(&zso->zos_disk, zv->zv_name, &zvol_dkdriver);
	zvol_set_geometry(zv, volsize);

	if (dmu_objset_is_snapshot(os) || !spa_writeable(dmu_objset_spa(os)))
		zv->zv_flags |= ZVOL_RDONLY;
	error = dataset_kstats_create(&zv->zv_kstat, os);
	if (error != 0)
		goto fail;

	zv->zv_zilog = zil_open(os, zvol_get_data, &zv->zv_kstat.dk_zil_sums);
	if (spa_writeable(dmu_objset_spa(os))) {
		if (zil_replay_disable)
			(void) zil_destroy(zv->zv_zilog, B_FALSE);
		else
			(void) zil_replay(os, zv, zvol_replay_vector);
	}
	zil_close(zv->zv_zilog);
	zv->zv_zilog = NULL;
	len = MIN(volsize, MIN(zvol_prefetch_bytes, SPA_MAXBLOCKSIZE));
	if (len != 0) {
		dmu_prefetch(os, ZVOL_OBJ, 0, 0, len, ZIO_PRIORITY_ASYNC_READ);
		dmu_prefetch(os, ZVOL_OBJ, 0, volsize - len, len,
		    ZIO_PRIORITY_ASYNC_READ);
	}
	error = zvol_create_nodes(name, m);
	if (error != 0)
		goto fail;

	/*
	 * Publish while still owning the objset, so a concurrent creator
	 * cannot own the same dataset before it appears in the name table.
	 * Native opens are blocked on zv_state_lock until disown completes.
	 */
	rw_enter(&zvol_state_lock, RW_WRITER);
	mutex_enter(&zv->zv_state_lock);
	zvol_insert(zv);
	disk_attach(&zso->zos_disk);
	mutex_enter(&zvol_dev_lock);
	list_insert_tail(&zvol_devices, zso);
	mutex_exit(&zvol_dev_lock);
	atomic_inc_uint(&zvol_minors);
	rw_exit(&zvol_state_lock);
	zv->zv_objset = NULL;
	dmu_objset_disown(os, B_TRUE, FTAG);
	mutex_exit(&zv->zv_state_lock);
	kmem_free(doi, sizeof (*doi));
	return (0);

fail:
	zvol_free_state(zv);
out:
	kmem_free(doi, sizeof (*doi));
	dmu_objset_disown(os, B_TRUE, FTAG);
	return (error);
}

void
zvol_os_remove_minor(zvol_state_t *zv)
{
	struct zvol_state_os *zso = zv->zv_zso;
	dev_info_t di = zvol_devinfo();

	ASSERT(MUTEX_HELD(&zv->zv_state_lock));
	ASSERT(zv->zv_flags & ZVOL_REMOVING);
	ASSERT0(zv->zv_open_count);
	mutex_exit(&zv->zv_state_lock);
	mutex_enter(&zvol_dev_lock);
	list_remove(&zvol_devices, zso);
	while (zso->zos_refs != 0)
		cv_wait(&zvol_dev_cv, &zvol_dev_lock);
	mutex_exit(&zvol_dev_lock);

	ddi_remove_minor_node(&di, zv->zv_name);
	disk_detach(&zso->zos_disk);
	mutex_enter(&zv->zv_state_lock);
}

void
zvol_os_free(zvol_state_t *zv)
{
	ASSERT0(zv->zv_open_count);
	ASSERT0P(zv->zv_objset);
	ASSERT0P(zv->zv_dn);
	ASSERT0P(zv->zv_zilog);
	zvol_free_state(zv);
	atomic_dec_uint(&zvol_minors);
}

int
zvol_os_rename_minor(zvol_state_t *zv, const char *newname)
{
	dev_info_t di = zvol_devinfo();
	struct zvol_state_os *zso = zv->zv_zso;
	int error;

	ASSERT(MUTEX_HELD(&zv->zv_state_lock));
	error = zvol_create_nodes(newname, zso->zos_minor);
	if (error != 0)
		return (error);
	ddi_remove_minor_node(&di, zv->zv_name);
	strlcpy(zv->zv_name, newname, sizeof (zv->zv_name));
	mutex_enter(&zso->zos_disk_lock);
	disk_rename(&zso->zos_disk, zv->zv_name);
	mutex_exit(&zso->zos_disk_lock);
	dataset_kstats_rename(&zv->zv_kstat, newname);
	return (0);
}

boolean_t
zvol_os_is_zvol(const char *path)
{
	vnode_t *vp;
	struct stat st;
	int error;

	/* Interior vdevs have no device path. */
	if (path == NULL)
		return (B_FALSE);

	error = namei_simple_kernel(path, NSM_FOLLOW_NOEMULROOT, &vp);
	if (error != 0)
		return (B_FALSE);
	vn_lock(vp, LK_SHARED | LK_RETRY);
	error = vn_stat(vp, &st);
	VOP_UNLOCK(vp);
	vrele(vp);
	return (error == 0 && minor(st.st_rdev) != 0 &&
	    ((S_ISBLK(st.st_mode) && major(st.st_rdev) == zfs_bmajor) ||
	    (S_ISCHR(st.st_mode) && major(st.st_rdev) == zfs_cmajor)));
}

int
zvol_os_update_volsize(zvol_state_t *zv, uint64_t volsize)
{
	zv->zv_volsize = volsize;
	zvol_set_geometry(zv, volsize);
	return (0);
}

void
zvol_os_set_capacity(zvol_state_t *zv, uint64_t capacity)
{
	zvol_set_geometry(zv, capacity << DEV_BSHIFT);
}

void
zvol_os_set_disk_ro(zvol_state_t *zv, int flags)
{
	/* Native I/O checks the common ZVOL_RDONLY flag. */
}

void
zvol_wait_close(zvol_state_t *zv)
{
	ASSERT(MUTEX_HELD(&zv->zv_state_lock));
	while (zv->zv_open_count != 0)
		cv_wait(&zv->zv_removing_cv, &zv->zv_state_lock);
}

int
zvol_busy(void)
{
	return (zvol_minors != 0);
}

int
zvol_init(void)
{
	int error;

	mutex_init(&zvol_dev_lock, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&zvol_dev_cv, NULL, CV_DEFAULT, NULL);
	list_create(&zvol_devices, sizeof (struct zvol_state_os),
	    offsetof(struct zvol_state_os, zos_link));
	zvol_minor_arena = vmem_create("zvolminor", 1, ZFSDEV_MAX_MINOR, 1,
	    NULL, NULL, NULL, 0, VM_SLEEP, IPL_NONE);
	error = zvol_init_impl();
	if (error != 0) {
		vmem_destroy(zvol_minor_arena);
		list_destroy(&zvol_devices);
		cv_destroy(&zvol_dev_cv);
		mutex_destroy(&zvol_dev_lock);
	}
	return (error);
}

void
zvol_fini(void)
{
	zvol_fini_impl();
	ASSERT(list_is_empty(&zvol_devices));
	vmem_destroy(zvol_minor_arena);
	list_destroy(&zvol_devices);
	cv_destroy(&zvol_dev_cv);
	mutex_destroy(&zvol_dev_lock);
}

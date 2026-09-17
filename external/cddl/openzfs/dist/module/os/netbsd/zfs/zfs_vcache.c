/* SPDX-License-Identifier: CDDL-1.0 */
/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Vnode construction follows the NetBSD integration in osnet/zfs_znode.c.
 * vcache serializes lookup, construction and reclamation by object number.
 * The shared znode metadata routines never publish a vnode themselves.
 */
#include <sys/zfs_context.h>
#include <sys/zfs_ctldir.h>
#include <sys/zfs_dir.h>
#include <sys/zfs_vcache.h>
#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>
#include <uvm/uvm_pager.h>

struct zfs_newvnode_args {
	dmu_tx_t *tx;
	uint_t flag;
	zfs_acl_ids_t *acl_ids;
};

int
zfs_loadvnode(struct mount *mp, vnode_t *vp, const void *key,
    size_t key_len, const void **new_key)
{
	zfsvfs_t *zfsvfs = mp->mnt_data;
	uint64_t obj;
	dmu_buf_t *db;
	dmu_object_info_t doi;
	znode_t *zp;
	int error;

	if (key_len != sizeof (obj))
		return (zfsctl_loadvnode(mp, vp, key, key_len, new_key));
	memcpy(&obj, key, sizeof (obj));
	ZFS_OBJ_HOLD_ENTER(zfsvfs, obj);
	error = sa_buf_hold(zfsvfs->z_os, obj, NULL, &db);
	if (error != 0)
		goto out;
	dmu_object_info_from_db(db, &doi);
	if (doi.doi_bonus_type != DMU_OT_SA &&
	    (doi.doi_bonus_type != DMU_OT_ZNODE ||
	    doi.doi_bonus_size < sizeof (znode_phys_t))) {
		sa_buf_rele(db, NULL);
		error = SET_ERROR(EINVAL);
		goto out;
	}
	if (dmu_buf_get_user(db) != NULL) {
		sa_buf_rele(db, NULL);
		error = SET_ERROR(ENOENT);
		goto out;
	}
	/* The SA handle takes ownership of the bonus buffer hold. */
	zp = zfs_znode_alloc(zfsvfs, db, doi.doi_data_block_size,
	    doi.doi_bonus_type, NULL, vp);
	if (zp == NULL) {
		error = SET_ERROR(ENOENT);
		goto out;
	}
	zp->z_dnodesize = doi.doi_dnodesize;
	if (zfsvfs->z_use_namecache)
		cache_enter_id(vp, zp->z_mode, zp->z_uid, zp->z_gid, true);
	*new_key = &zp->z_id;
out:
	ZFS_OBJ_HOLD_EXIT(zfsvfs, obj);
	return (error);
}

int
zfs_newvnode(struct mount *mp, vnode_t *dvp, vnode_t *vp, vattr_t *vap,
    cred_t *cr, void *extra, size_t *key_len, const void **new_key)
{
	struct zfs_newvnode_args *args = extra;
	znode_t *dzp = VTOZ(dvp), *zp;

	zfs_mknode_impl(dzp, vap, args->tx, cr, args->flag, &zp,
	    args->acl_ids, vp);
	if (dzp->z_zfsvfs->z_use_namecache)
		cache_enter_id(vp, zp->z_mode, zp->z_uid, zp->z_gid, true);
	*key_len = sizeof (zp->z_id);
	*new_key = &zp->z_id;
	return (0);
}

void
zfs_mknode(znode_t *dzp, vattr_t *vap, dmu_tx_t *tx, cred_t *cr,
    uint_t flag, znode_t **zpp, zfs_acl_ids_t *acl_ids)
{
	struct zfs_newvnode_args args = { tx, flag, acl_ids };
	vnode_t *vp;

	if (flag & IS_ROOT_NODE) {
		zfs_mknode_impl(dzp, vap, tx, cr, flag, zpp, acl_ids, NULL);
		return;
	}
	VERIFY0(vcache_new(dzp->z_zfsvfs->z_vfs, ZTOV(dzp), vap, cr,
	    &args, &vp));
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	*zpp = VTOZ(vp);
}

int
zfs_zget(zfsvfs_t *zfsvfs, uint64_t obj, znode_t **zpp)
{
	vnode_t *vp;
	int error;

	*zpp = NULL;
	error = vcache_get(zfsvfs->z_vfs, &obj, sizeof (obj), &vp);
	if (error == 0)
		*zpp = VTOZ(vp);
	return (error);
}

void
zfs_netbsd_setsize(vnode_t *vp, uint64_t size)
{
	struct uvm_object *uobj = &vp->v_uobj;
	struct vm_page *pg = NULL;
	vaddr_t va;
	int count = 1;
	size_t pgoff = size & PAGE_MASK;

	uvm_vnp_setsize(vp, size);
	if (!vn_has_cached_data(vp) || pgoff == 0)
		return;

	/*
	 * UVM removes whole pages beyond EOF.  As in osnet, also zero the
	 * tail of a cached partial page so later growth cannot expose it.
	 */
	rw_enter(uobj->vmobjlock, RW_WRITER);
	if (uvn_findpages(uobj, trunc_page(size), &count, &pg, NULL,
	    UFP_NOALLOC)) {
		va = uvm_pagermapin(&pg, 1,
		    UVMPAGER_MAPIN_WAITOK | UVMPAGER_MAPIN_READ);
		memset((char *)va + pgoff, 0, PAGE_SIZE - pgoff);
		uvm_pagermapout(va, 1);
		uvm_page_unbusy(&pg, 1);
	}
	rw_exit(uobj->vmobjlock);
}

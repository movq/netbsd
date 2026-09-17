/* SPDX-License-Identifier: CDDL-1.0 */
/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 *
 * NetBSD filehandles, adapted from osnet. Native fid_len and the size passed
 * to VFS_VPTOFH include the length field. The old subtraction of that field
 * underreported the buffer size by two bytes.
 */
#include <sys/zfs_context.h>
#include <sys/zfs_ctldir.h>
#include <sys/zfs_ioctl_impl.h>
#include <sys/dmu_objset.h>

static uint64_t
zfs_fid_decode(const uint8_t *p, size_t len)
{
	uint64_t value = 0;
	for (size_t i = 0; i < len; i++)
		value |= (uint64_t)p[i] << (8 * i);
	return (value);
}

static void
zfs_fid_encode(uint8_t *p, size_t len, uint64_t value)
{
	for (size_t i = 0; i < len; i++)
		p[i] = value >> (8 * i);
}

int
zfs_netbsd_vptofh(vnode_t *vp, fid_t *fid, size_t *size)
{
	znode_t *zp;
	zfsvfs_t *zfsvfs;
	zfid_long_t out;
	size_t needed;
	uint64_t gen;
	int error;

	if (zfsctl_is_node(vp))
		return (zfsctl_vptofh(vp, fid, size));
	zp = VTOZ(vp);
	zfsvfs = zp->z_zfsvfs;
	needed = zfsvfs->z_parent == zfsvfs ? SHORT_FID_LEN : LONG_FID_LEN;
	if (*size < needed) {
		*size = needed;
		return (SET_ERROR(E2BIG));
	}
	*size = needed;
	if ((error = zfs_enter_verify_zp(zfsvfs, zp, FTAG)) != 0)
		return (error);
	error = sa_lookup(zp->z_sa_hdl, SA_ZPL_GEN(zfsvfs), &gen, sizeof (gen));
	if (error != 0)
		goto out;
	memset(&out, 0, sizeof (out));
	out.z_fid.zf_len = needed;
	zfs_fid_encode(out.z_fid.zf_object, sizeof (out.z_fid.zf_object),
	    zp->z_id);
	/* Generation zero is reserved for the synthetic control directory. */
	gen = (uint32_t)gen;
	if (gen == 0)
		gen = 1;
	zfs_fid_encode(out.z_fid.zf_gen, sizeof (out.z_fid.zf_gen), gen);
	if (needed == LONG_FID_LEN)
		zfs_fid_encode(out.zf_setid, sizeof (out.zf_setid),
		    dmu_objset_id(zfsvfs->z_os));
	memcpy(fid, &out, needed);
out:
	zfs_exit(zfsvfs, FTAG);
	return (error);
}

int
zfs_netbsd_fhtovp(vfs_t *mp, fid_t *fid, int flags, vnode_t **vpp)
{
	zfsvfs_t *zfsvfs = mp->mnt_data;
	zfid_long_t in;
	znode_t *zp;
	uint64_t object, generation, actual;
	boolean_t snapshot_ref = B_FALSE;
	int error;

	*vpp = NULL;
	if (fid->fid_len != SHORT_FID_LEN && fid->fid_len != LONG_FID_LEN)
		return (SET_ERROR(EINVAL));
	memset(&in, 0, sizeof (in));
	memcpy(&in, fid, fid->fid_len);
	if (in.z_fid.zf_len == LONG_FID_LEN && zfsvfs->z_parent == zfsvfs) {
		uint64_t setid = zfs_fid_decode(in.zf_setid,
		    sizeof (in.zf_setid));
		error = zfsctl_lookup_objset(mp, setid, &zfsvfs);
		if (error != 0)
			return (SET_ERROR(ESTALE));
		snapshot_ref = B_TRUE;
	}
	if ((error = zfs_enter(zfsvfs, FTAG)) != 0)
		goto out;
	if (in.z_fid.zf_len == LONG_FID_LEN &&
	    (zfs_fid_decode(in.zf_setid, sizeof (in.zf_setid)) !=
	    dmu_objset_id(zfsvfs->z_os) ||
	    zfs_fid_decode(in.zf_setgen, sizeof (in.zf_setgen)) != 0)) {
		error = SET_ERROR(ESTALE);
		goto exit;
	}
	object = zfs_fid_decode(in.z_fid.zf_object, sizeof (in.z_fid.zf_object));
	generation = zfs_fid_decode(in.z_fid.zf_gen, sizeof (in.z_fid.zf_gen));
	if (generation == 0 &&
	    (object == ZFSCTL_INO_ROOT || object == ZFSCTL_INO_SNAPDIR)) {
		error = object == ZFSCTL_INO_ROOT ?
		    zfsctl_root(zfsvfs, vpp) : zfsctl_snapshot(zfsvfs, vpp);
		goto exit;
	}
	error = zfs_zget(zfsvfs, object, &zp);
	if (error != 0) {
		error = SET_ERROR(ESTALE);
		goto exit;
	}
	if (zp->z_sa_hdl == NULL || zp->z_unlinked) {
		error = SET_ERROR(ESTALE);
	} else {
		error = sa_lookup(zp->z_sa_hdl, SA_ZPL_GEN(zfsvfs),
		    &actual, sizeof (actual));
		if (error == 0) {
			actual = (uint32_t)actual;
			if (actual == 0)
				actual = 1;
			if (actual != generation)
				error = SET_ERROR(ESTALE);
		}
	}
	if (error != 0)
		vrele(ZTOV(zp));
	else
		*vpp = ZTOV(zp);
exit:
	zfs_exit(zfsvfs, FTAG);
	if (error == 0) {
		error = vn_lock(*vpp, flags);
		if (error != 0) {
			vrele(*vpp);
			*vpp = NULL;
		}
	}
out:
	if (snapshot_ref)
		zfs_vfs_rele(zfsvfs);
	return (error);
}

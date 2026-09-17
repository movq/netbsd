/* SPDX-License-Identifier: BSD-2-Clause */
#include "fid_shim.h"
#include <stdio.h>
static objset_t os = { 42 }, snapos = { UINT64_C(0xabcdef987654) };
static zfsvfs_t fs = { .z_os = &os }, snapfs = { .z_os = &snapos };
static uint64_t gen = UINT64_C(0x1234fedcba98);
static znode_t zp = { .z_zfsvfs = &fs,
    .z_id = UINT64_C(0x123456789abc), .z_sa_hdl = &gen };
static vnode_t vp = { .node = &zp };
static int sa_error, lock_error;

int zfs_enter(zfsvfs_t *f, const char *tag)
{ assert(!f->entered); f->entered++; return 0; }
int zfs_enter_verify_zp(zfsvfs_t *f, znode_t *z, const char *tag)
{ assert(z->z_sa_hdl); return zfs_enter(f, tag); }
void zfs_exit(zfsvfs_t *f, const char *tag)
{ assert(f->entered == 1); f->entered--; }
int sa_lookup(uint64_t *hdl, int attr, void *out, size_t len)
{ assert(len == 8); if (!sa_error) memcpy(out, hdl, len); return sa_error; }
int zfsctl_vptofh(vnode_t *v, fid_t *fid, size_t *size)
{ abort(); }
int zfsctl_root(zfsvfs_t *f, vnode_t **v) { return ENOENT; }
int zfsctl_snapshot(zfsvfs_t *f, vnode_t **v) { return ENOENT; }
int
zfsctl_lookup_objset(vfs_t *mp, uint64_t id, zfsvfs_t **out)
{
	if (id != snapos.id)
		return ENOENT;
	snapfs.busy++;
	*out = &snapfs;
	return 0;
}
int
zfs_zget(zfsvfs_t *f, uint64_t id, znode_t **out)
{
	assert(f->entered);
	if (id != zp.z_id)
		return ENOENT;
	*out = &zp;
	vp.refs++;
	return 0;
}
void zfs_vfs_rele(zfsvfs_t *f)
{ assert(f->busy); f->busy--; }
void vrele(vnode_t *v) { assert(v->refs); v->refs--; }
int vn_lock(vnode_t *v, int flags)
{ if (!lock_error) v->locked = true; return lock_error; }

static void
roundtrip(bool snapshot)
{
	size_t size = 0;
	vnode_t *out;
	vfs_t mp = { &fs };
	zp.z_zfsvfs = snapshot ? &snapfs : &fs;
	assert(zfs_netbsd_vptofh(&vp, NULL, &size) == E2BIG);
	assert(size == (snapshot ? sizeof (zfid_long_t) : sizeof (zfid_short_t)));
	fid_t *fid = malloc(size);
	assert(fid);
	assert(zfs_netbsd_vptofh(&vp, fid, &size) == 0);
	assert(fid->fid_len == size);
	assert(zfs_netbsd_fhtovp(&mp, fid, 1, &out) == 0);
	assert(out == &vp && vp.locked && vp.refs == 1);
	assert(!snapfs.busy && !fs.entered && !snapfs.entered);
	vp.locked = false;
	vrele(out);

	gen += 2;
	assert(zfs_netbsd_fhtovp(&mp, fid, 1, &out) == ESTALE && !out);
	gen -= 2;
	assert(!vp.refs && !snapfs.busy);
	zp.z_unlinked = true;
	assert(zfs_netbsd_fhtovp(&mp, fid, 1, &out) == ESTALE && !out);
	zp.z_unlinked = false;
	sa_error = EIO;
	assert(zfs_netbsd_fhtovp(&mp, fid, 1, &out) == EIO && !out);
	sa_error = 0;
	lock_error = EBUSY;
	assert(zfs_netbsd_fhtovp(&mp, fid, 1, &out) == EBUSY && !out);
	lock_error = 0;
	assert(!vp.refs && !snapfs.busy);
	if (snapshot) {
		((zfid_long_t *)fid)->zf_setgen[0] = 1;
		assert(zfs_netbsd_fhtovp(&mp, fid, 1, &out) == ESTALE);
		assert(!snapfs.busy);
	}
	free(fid);
}

int
main(void)
{
	vfs_t mp = { &fs };
	vnode_t *out;
	fs.z_parent = &fs;
	snapfs.z_parent = &fs;
	zp.vp = &vp;
	roundtrip(false);
	roundtrip(true);
	gen = UINT64_C(1) << 32; /* Zero truncated generation becomes one. */
	roundtrip(false);
	/* Malformed old/truncated lengths must fail before reading the body. */
	for (uint16_t len = 0; len < sizeof (zfid_long_t) + 2; len++) {
		if (len == SHORT_FID_LEN || len == LONG_FID_LEN)
			continue;
		fid_t *fid = malloc(sizeof (*fid));
		fid->fid_len = len;
		assert(zfs_netbsd_fhtovp(&mp, fid, 1, &out) == EINVAL && !out);
		free(fid);
	}
	puts("PASS: filehandle bounds, generations, snapshots and error references");
	return 0;
}

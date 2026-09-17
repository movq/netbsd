/* SPDX-License-Identifier: CDDL-1.0 */
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct { int entered; } zfsvfs_t;
typedef struct vnode vnode_t;
typedef struct znode {
	zfsvfs_t *z_zfsvfs;
	vnode_t *vp;
} znode_t;
struct vnode { znode_t z; int refs, locked; };
typedef int cred_t, vattr_t, zidmap_t;
struct componentname {
	char *cn_nameptr;
	size_t cn_namelen;
	cred_t *cn_cred;
};
#define ZTOV(zp) ((zp)->vp)
#define __UNCONST(p) ((char *)(p))
#define FTAG NULL
#define LK_EXCLUSIVE 1
#define LK_NOWAIT 2
#define ZEXISTS 1
static vnode_t *source, *target, *fail_lock;
static int fail_errno, enter_error, verify_error, lookup_error;
static int attempts, calls;
static vnode_t *replacement;
static int
vn_lock(vnode_t *vp, int flags)
{
	assert(vp->refs > 0 && !vp->locked);
	if (vp == fail_lock && (flags & LK_NOWAIT)) {
		fail_lock = NULL;
		return fail_errno;
	}
	vp->locked = 1;
	return 0;
}
static void VOP_UNLOCK(vnode_t *vp)
{
	assert(vp->locked);
	vp->locked = 0;
}
static void vref(vnode_t *vp) { assert(vp->refs > 0); vp->refs++; }
static void vrele(vnode_t *vp) { assert(vp->refs > 1); vp->refs--; }
static int is_nametoolong(zfsvfs_t *fs, const char *name)
{
	return strlen(name) > 255;
}
static int zfs_enter_verify_zp(zfsvfs_t *fs, znode_t *zp, void *tag)
{
	if (enter_error)
		return enter_error;
	assert(!fs->entered);
	fs->entered++;
	return 0;
}
static int zfs_verify_zp(znode_t *zp) { return verify_error; }
static void zfs_exit(zfsvfs_t *fs, void *tag)
{
	assert(fs->entered == 1);
	fs->entered--;
}
static int
zfs_dirent_lookup(znode_t *dir, const char *name, znode_t **out, int flags)
{
	assert(dir->vp->locked && dir->z_zfsvfs->entered);
	if (lookup_error)
		return lookup_error;
	vnode_t *vp;
	if (flags == ZEXISTS) {
		attempts++;
		if (attempts > 1 && replacement != NULL)
			source = replacement;
		vp = source;
	} else {
		vp = target;
	}
	if (vp) {
		vref(vp);
		*out = &vp->z;
	} else {
		*out = NULL;
	}
	return 0;
}
static int
zfs_do_rename_impl(vnode_t *sd, vnode_t **sv, struct componentname *sc,
    vnode_t *td, vnode_t **tv, struct componentname *tc, cred_t *cr)
{
	assert(sd->locked && td->locked && (*sv)->locked);
	assert(*tv == NULL || (*tv)->locked);
	assert(*sv == source && *tv == target);
	assert(!sd->z.z_zfsvfs->entered);
	calls++;
	return 0;
}
#include "rename.h"

int main(void)
{
	zfsvfs_t fs = { 0 }, other = { 0 };
	vnode_t nodes[5];
	for (unsigned scenario = 0; scenario < 15; scenario++) {
		for (unsigned i = 0; i < 5; i++)
			nodes[i] = (vnode_t) { { &fs, &nodes[i] }, 1, 0 };
		znode_t *sd = &nodes[0].z, *td = &nodes[1].z;
		source = &nodes[2];
		target = &nodes[3];
		replacement = fail_lock = NULL;
		fail_errno = EBUSY;
		enter_error = verify_error = lookup_error = 0;
		attempts = calls = 0;
		int expected = 0, want_calls = 1;
		switch (scenario) {
		case 0: break; /* Four distinct vnodes. */
		case 1: td = sd; break;
		case 2: target = source; break;
		case 3: target = NULL; break;
		case 4: fail_lock = &nodes[1]; break;
		case 5: fail_lock = source; break;
		case 6: fail_lock = target; break;
		case 7:
			fail_lock = source;
			replacement = &nodes[4];
			break;
		case 8:
			nodes[1].z.z_zfsvfs = &other;
			expected = EXDEV; want_calls = 0;
			break;
		case 9:
			enter_error = expected = EIO; want_calls = 0;
			break;
		case 10:
			verify_error = expected = EIO; want_calls = 0;
			break;
		case 11:
			lookup_error = expected = ENOENT; want_calls = 0;
			break;
		case 12:
			fail_lock = target;
			fail_errno = expected = ENOENT; want_calls = 0;
			break;
		case 13:
			source = &nodes[1];
			expected = EINVAL; want_calls = 0;
			break;
		case 14:
			target = &nodes[0];
			expected = EINVAL; want_calls = 0;
			break;
		}
		assert(zfs_rename(sd, "from", td, "to", NULL, 0, 0, NULL,
		    NULL) == expected);
		assert(calls == want_calls && !fs.entered && !other.entered);
		if (scenario == 7)
			assert(attempts == 2 && source == replacement);
		for (unsigned i = 0; i < 5; i++)
			assert(nodes[i].refs == 1 && !nodes[i].locked);
	}
	puts("native rename lock/reference tests passed");
	return 0;
}

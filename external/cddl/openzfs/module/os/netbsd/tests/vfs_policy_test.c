/* SPDX-License-Identifier: BSD-2-Clause */
#include "vfs_policy_shim.h"
#include <stdio.h>

static dsl_dir_t dir;
static dsl_dataset_t dataset = { .ds_dir = &dir };
static bool feature_enabled;
static objset_t os = { &dataset, &feature_enabled };
static vfs_t mountstate;
static zfsvfs_t *fs;
static znode_t znodes[2];
static unsigned inits, setups, finishes, holds, releases, unmounts, warnings;
static int init_error, setup_error, unmount_error;
static bool failed;

void cmn_err(int level, const char *fmt, ...)
{ if (level == CE_WARN) warnings++; }

objset_t *
zfs_suspended_objset(zfsvfs_t *f, dsl_dataset_t *ds)
{
	assert(f == fs && ds == &dataset);
	f->z_os = &os;
	return &os;
}
int zfsvfs_init(zfsvfs_t *f, objset_t *o)
{ assert(f == fs && o == &os); inits++; return init_error; }
int zfsvfs_setup(zfsvfs_t *f, bool mounting)
{ assert(f == fs && !mounting); setups++; return setup_error; }
int zfs_rezget(znode_t *zp)
{ assert(fs->z_znodes_lock); zp->rebound = true; return 0; }
void zfs_resume_finish(zfsvfs_t *f, bool error)
{ assert(f == fs && !f->z_znodes_lock); finishes++; failed = error; }
int zfsvfs_hold(const char *name, const char *tag, zfsvfs_t **out, bool writer)
{ holds++; *out = fs; return 0; }
void zfsvfs_rele(zfsvfs_t *f, const char *tag)
{ assert(f == fs); releases++; }
void vfs_ref(vfs_t *mp) { mp->refs++; }
void vfs_rele(vfs_t *mp) { assert(mp->refs); mp->refs--; }
void vfs_unbusy(vfs_t *mp)
{ assert(mp->busy && mp->refs); mp->busy--; mp->refs--; }
int
dounmount(vfs_t *mp, int flags, void *lwp)
{
	assert(mp->busy == 0 && mp->refs == 1 && flags == MNT_FORCE);
	assert(fs && !fs->z_lock);
	unmounts++;
	if (!unmount_error) {
		mp->unmounted = true;
		mp->fs = NULL;
		free(fs); /* ASan catches accesses after this call returns. */
		fs = NULL;
	}
	return unmount_error;
}

static void
reset(void)
{
	free(fs);
	fs = calloc(1, sizeof (*fs));
	assert(fs);
	memset(&mountstate, 0, sizeof (mountstate));
	memset(znodes, 0, sizeof (znodes));
	mountstate.refs = mountstate.busy = 1;
	mountstate.fs = fs;
	fs->z_vfs = &mountstate;
	fs->z_os = &os;
	fs->z_all_znodes = &znodes[0];
	znodes[0].next = &znodes[1];
	inits = setups = finishes = holds = releases = unmounts = warnings = 0;
	init_error = setup_error = unmount_error = 0;
	dataset.longname_active = false;
	dir.dd_activity_cancelled = true;
	feature_enabled = true;
	failed = false;
}

int
main(void)
{
	reset();
	/* The enabled pool feature alone does not prevent mounting. */
	assert(zfs_netbsd_check_mount(&os) == 0);
	dataset.longname_active = true;
	/* A recorded use blocks access independently of current properties. */
	assert(zfs_netbsd_check_mount(&os) == ENOTSUP);
	assert(zfs_resume_fs(fs, &dataset) == ENOTSUP);
	assert(failed && finishes == 1 && !inits && !setups);
	assert(!znodes[0].rebound && dir.dd_activity_cancelled);

	reset();
	assert(zfs_resume_fs(fs, &dataset) == 0);
	assert(inits == 1 && setups == 1 && finishes == 1 && !failed);
	assert(znodes[0].rebound && znodes[1].rebound);
	assert(!dir.dd_activity_cancelled);
	reset();
	init_error = EIO;
	assert(zfs_resume_fs(fs, &dataset) == EIO);
	assert(failed && !setups && !znodes[0].rebound);
	reset();
	setup_error = ENOMEM;
	assert(zfs_resume_fs(fs, &dataset) == ENOMEM);
	assert(failed && setups == 1 && !znodes[0].rebound);

	reset();
	assert(set_longname(ZPROP_SRC_LOCAL, 1) == ENOTSUP);
	assert(!holds);
	assert(set_longname(ZPROP_SRC_LOCAL, 0) == -1);
	assert(holds == 1 && releases == 1);
	assert(set_longname(ZPROP_SRC_RECEIVED, 1) == -1);
	assert(holds == 1); /* Receive metadata bypasses filesystem holds. */
	assert(set_longname(ZPROP_SRC_NONE, 1) == -1);
	assert(holds == 2 && releases == 2);

	reset();
	zfs_vfs_rele(fs);
	assert(!unmounts && !mountstate.busy && !mountstate.refs);
	reset();
	fs->z_unmount_pending = true;
	zfs_vfs_rele(fs);
	assert(!fs && mountstate.unmounted && unmounts == 1);
	assert(!mountstate.refs);
	reset();
	fs->z_unmount_pending = true;
	unmount_error = EBUSY;
	zfs_vfs_rele(fs);
	assert(fs && !fs->z_unmount_pending && warnings == 1);
	assert(!mountstate.refs && !mountstate.busy);
	free(fs);
	fs = NULL;
	puts("PASS: longname property/mount policy, resume failures, VFS release");
	return 0;
}

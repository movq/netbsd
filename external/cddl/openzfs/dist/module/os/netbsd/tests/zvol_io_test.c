/* SPDX-License-Identifier: BSD-2-Clause */
#include "zvol_io_shim.h"

static zvol_state_t zv;
static struct zvol_state_os zso;
static objset_t os;
static zil_header_t zh;
static zilog_t logstate = { &zh };
static int open_error, io_error, assign_error, flush_error;
static unsigned holds, first_opens, last_closes, commits, completions, busy;
static size_t partial, transferred, logged;
static int tx, range;

zvol_state_t *
zvol_os_hold(dev_t dev, boolean_t closing)
{
	if (dev != 1 || (!closing && (zv.zv_flags & ZVOL_REMOVING)))
		return NULL;
	assert(!holds && !zv.zv_state_lock && !zv.zv_suspend_lock);
	holds++;
	zv.zv_suspend_lock = 1;
	mutex_enter(&zv.zv_state_lock);
	return &zv;
}

void
zvol_os_rele(zvol_state_t *v)
{
	assert(v == &zv && holds == 1 && zv.zv_suspend_lock == 1);
	mutex_exit(&zv.zv_state_lock);
	zv.zv_suspend_lock = 0;
	holds--;
}

int zvol_first_open(zvol_state_t *v, boolean_t ro)
{ first_opens++; if (!open_error) v->zv_objset = &os; return open_error; }
void zvol_last_close(zvol_state_t *v)
{ last_closes++; v->zv_objset = NULL; v->zv_zilog = NULL; }
zilog_t *zil_open(objset_t *o, void *f, int *s) { return &logstate; }
int zil_commit(zilog_t *z, uint64_t obj)
{ assert(z == &logstate); commits++; return flush_error; }
void disk_busy(struct disk *d) { assert(!busy); busy++; }
void disk_unbusy(struct disk *d, size_t n, int read)
{ assert(busy == 1); busy--; }
int disk_ioctl(struct disk *d, dev_t dev, unsigned long cmd, void *p,
    int f, lwp_t *l) { return EPASSTHROUGH; }
void biodone(struct buf *b) { completions++; }

zfs_locked_range_t *
zfs_rangelock_enter(int *r, uint64_t off, uint64_t len, int type)
{
	assert(!range && len && off + len <= zv.zv_volsize);
	assert(!zv.zv_state_lock && zv.zv_suspend_lock);
	range = 1;
	return &range;
}
void zfs_rangelock_exit(zfs_locked_range_t *r)
{ assert(*r == 1); *r = 0; }

static int
transfer(zfs_uio_t *u, uint64_t size)
{
	size_t n = io_error ? MIN(size, partial) : size;
	assert((uint64_t)u->uio->uio_offset + size <= zv.zv_volsize);
	assert(size <= DMU_MAX_ACCESS / 2 && range && !zv.zv_state_lock);
	u->uio->uio_offset += n;
	u->uio->uio_resid -= n;
	transferred += n;
	return io_error;
}
int dmu_read_uio_dnode(dnode_t *d, zfs_uio_t *u, uint64_t n, int f)
{ return transfer(u, n); }
dmu_tx_t *dmu_tx_create(objset_t *o) { assert(!tx); tx = 1; return &tx; }
void dmu_tx_hold_write_by_dnode(dmu_tx_t *t, dnode_t *d, uint64_t o, uint64_t n)
{ assert(o + n <= zv.zv_volsize); }
int dmu_tx_assign(dmu_tx_t *t, int flags) { return assign_error; }
void dmu_tx_abort(dmu_tx_t *t) { assert(*t); *t = 0; }
void dmu_tx_commit(dmu_tx_t *t) { assert(*t); *t = 0; }
int dmu_write_uio_dnode(dnode_t *d, zfs_uio_t *u, uint64_t n, dmu_tx_t *t, int f)
{ assert(*t); return transfer(u, n); }
void zvol_log_write(zvol_state_t *v, dmu_tx_t *t, uint64_t o, uint64_t n,
    boolean_t sync) { assert(*t && n); logged += n; }
void dataset_kstats_update_read_kstats(dataset_kstats_t *k, int64_t n) {}
void dataset_kstats_update_write_kstats(dataset_kstats_t *k, int64_t n) {}

static void
reset(void)
{
	assert(!holds && !range && !tx && !busy);
	memset(&zv, 0, sizeof zv);
	memset(&zso, 0, sizeof zso);
	memset(&os, 0, sizeof os);
	zv.zv_zso = &zso;
	zv.zv_volsize = 8192;
	open_error = io_error = assign_error = flush_error = 0;
	partial = transferred = logged = 0;
	first_opens = last_closes = commits = completions = 0;
}

static void
test_opens(void)
{
	reset();
	open_error = EIO;
	assert(zvol_open(1, FWRITE, S_IFBLK, NULL) == EIO);
	assert(!last_closes && !zv.zv_open_count);
	open_error = 0;
	assert(zvol_open(1, FWRITE, S_IFBLK, NULL) == 0);
	assert(zvol_open(1, FWRITE, S_IFBLK, NULL) == 0);
	assert(zvol_open(1, FWRITE, S_IFCHR, NULL) == 0);
	assert(first_opens == 2 && zv.zv_open_count == 2);
	assert(zvol_open(1, FWRITE | O_EXCL, S_IFCHR, NULL) == EBUSY);
	zv.zv_flags |= ZVOL_REMOVING;
	assert(zvol_open(1, 0, S_IFCHR, NULL) == ENXIO);
	assert(zvol_close(1, 0, S_IFBLK, NULL) == 0);
	assert(!last_closes && zv.zv_open_count == 1);
	assert(zvol_close(1, 0, S_IFCHR, NULL) == 0);
	assert(last_closes == 1 && !zv.zv_open_count);

	reset();
	zv.zv_flags |= ZVOL_RDONLY;
	assert(zvol_open(1, FWRITE, S_IFCHR, NULL) == EROFS);
	assert(first_opens == 1 && last_closes == 1);
}

static struct uio
make_uio(enum uio_rw rw, off_t offset, size_t count)
{
	struct uio u = { .uio_rw = rw, .uio_offset = offset, .uio_resid = count };
	return u;
}

static void
test_io(void)
{
	struct uio u;
	reset();
	assert(zvol_open(1, FWRITE, S_IFCHR, NULL) == 0);
	u = make_uio(UIO_WRITE, 1024, 5000);
	assert(zvol_write(1, &u, IO_SYNC) == 0);
	assert(!u.uio_resid && logged == 5000 && transferred == 5000);
	assert(commits == 1);
	u = make_uio(UIO_READ, 8000, 1000);
	assert(zvol_read(1, &u, 0) == 0);
	assert(u.uio_resid == 808 && u.uio_offset == 8192);
	u = make_uio(UIO_READ, 8192, 1000);
	assert(zvol_read(1, &u, 0) == 0 && u.uio_resid == 1000);
	u = make_uio(UIO_WRITE, 8192, 1000);
	assert(zvol_write(1, &u, 0) == ENOSPC);
	u = make_uio(UIO_READ, -1, 1000);
	assert(zvol_read(1, &u, 0) == EIO);

	io_error = EFAULT;
	partial = 100;
	u = make_uio(UIO_WRITE, 100, 500);
	assert(zvol_write(1, &u, IO_SYNC) == EFAULT);
	assert(u.uio_resid == 400 && logged == 5100 && commits == 2);
	io_error = ECKSUM;
	partial = 0;
	u = make_uio(UIO_READ, 100, 500);
	assert(zvol_read(1, &u, 0) == EIO);
	io_error = 0;
	assign_error = ENOSPC;
	u = make_uio(UIO_WRITE, 100, 500);
	assert(zvol_write(1, &u, 0) == ENOSPC && u.uio_resid == 500);
	assert(logged == 5100);
	zv.zv_flags |= ZVOL_RDONLY;
	assert(zvol_write(1, &u, 0) == EROFS);
	zv.zv_flags &= ~ZVOL_RDONLY;
	flush_error = EIO;
	assert(zvol_ioctl(1, DIOCCACHESYNC, NULL, 0, NULL) == EIO);
	assert(zvol_close(1, 0, S_IFCHR, NULL) == 0);
}

static void
test_strategy(void)
{
	struct buf b = { .b_bcount = 4096, .b_blkno = 12, .b_flags = B_READ,
	    .b_dev = 1 };
	reset();
	assert(zvol_open(1, FWRITE, S_IFBLK, NULL) == 0);
	zvol_strategy(&b);
	assert(completions == 1 && !b.b_error && b.b_resid == 2048);
	b.b_blkno = INT64_MAX;
	zvol_strategy(&b);
	assert(completions == 2 && b.b_error == EINVAL && b.b_resid == 4096);
	b.b_blkno = 0;
	b.b_dev = 2;
	zvol_strategy(&b);
	assert(completions == 3 && b.b_error == ENXIO && b.b_resid == 4096);
	assert(zvol_close(1, 0, S_IFBLK, NULL) == 0);
}

int
main(void)
{
	test_opens();
	test_io();
	test_strategy();
	reset();
	puts("zvol I/O: open accounting, bounds, partial writes, and errors passed");
	return 0;
}

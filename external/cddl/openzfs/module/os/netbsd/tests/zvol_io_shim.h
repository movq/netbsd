/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef ZVOL_IO_SHIM_H
#define ZVOL_IO_SHIM_H
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>

typedef bool boolean_t;
typedef void lwp_t;
typedef int kmutex_t;
#define B_TRUE true
#define B_FALSE false
#define ASSERT(x) assert(x)
#define VERIFY0(x) assert((x) == 0)
#define ASSERT3U(a, op, b) assert((a) op (b))
#define MUTEX_HELD(p) (*(p) == 1)
#define RW_READ_HELD(p) (*(p) == 1)
#define SET_ERROR(x) (x)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define FTAG ((void *)"test")
#define FWRITE 2
#define IO_SYNC 1
#define ZVOL_RDONLY 1
#define ZVOL_WRITTEN_TO 2
#define ZVOL_EXCL 4
#define ZVOL_REMOVING 8
#define ZIL_REPLAY_NEEDED 1
#define ZFS_SYNC_ALWAYS 1
#define DMU_MAX_ACCESS 4096
#define DMU_READ_PREFETCH 0
#define DMU_TX_WAIT 0
#define ZVOL_OBJ 1
#define RL_READER 0
#define RL_WRITER 1
#define ECKSUM 1000
#define EPASSTHROUGH 1001
#define DIOCCACHESYNC 10
#define DIOCGWEDGEINFO 11
#define DIOCGPARTINFO 12
#define NODEV ((dev_t)-1)
#define DEV_BSHIFT 9
#define BLKDEV_IOSIZE 2048
#define FS_OTHER 1
#define DKW_PTYPE_FFS "ffs"
#define B_READ 1
#define B_ASYNC 2
enum uio_rw { UIO_READ, UIO_WRITE };
struct uio {
	struct iovec *uio_iov;
	int uio_iovcnt;
	off_t uio_offset;
	size_t uio_resid;
	enum uio_rw uio_rw;
};
#define UIO_SETUP_SYSSPACE(u) ((void)(u))
typedef struct { struct uio *uio; } zfs_uio_t;
typedef struct { int os_sync; } objset_t;
typedef struct { int zh_flags; } zil_header_t;
typedef struct { zil_header_t *zl_header; } zilog_t;
typedef struct { int dk_zil_sums; } dataset_kstats_t;
typedef int dnode_t;
typedef int dmu_tx_t;
typedef int zfs_locked_range_t;
struct disk_geom { unsigned dg_secsize; uint64_t dg_secperunit; };
struct disk { struct disk_geom dk_geom; };
struct zvol_state_os {
	unsigned zos_openmask;
	int zos_disk_lock;
	struct disk zos_disk;
};
typedef struct {
	unsigned zv_flags, zv_open_count;
	int zv_state_lock, zv_suspend_lock, zv_removing_cv, zv_rangelock;
	uint64_t zv_volsize;
	objset_t *zv_objset;
	zilog_t *zv_zilog;
	dnode_t *zv_dn;
	dataset_kstats_t zv_kstat;
	struct zvol_state_os *zv_zso;
	char zv_name[32];
} zvol_state_t;
struct buf {
	void *b_data;
	size_t b_bcount, b_resid;
	int b_flags, b_error;
	int64_t b_blkno;
	dev_t b_dev;
};
struct dkwedge_info {
	char dkw_devname[32], dkw_parent[32], dkw_ptype[16];
	uint64_t dkw_size;
};
struct partinfo {
	unsigned pi_secsize, pi_fstype, pi_bsize;
	uint64_t pi_size;
};
static inline void mutex_enter(int *m) { assert(!*m); *m = 1; }
static inline void mutex_exit(int *m) { assert(*m); *m = 0; }
#define strlcpy zvol_test_strlcpy
static inline size_t strlcpy(char *d, const char *s, size_t n)
{ size_t len = strlen(s); if (n) snprintf(d, n, "%s", s); return len; }
static inline void zfs_uio_init(zfs_uio_t *z, struct uio *u) { z->uio = u; }
static inline int spa_namespace_held(void) { return 1; }
static inline int spa_namespace_tryenter(const void *t) { return 1; }
static inline void spa_namespace_exit(const void *t) {}
static inline void kpause(const char *s, bool b, int t, void *p) {}
static inline void cv_broadcast(int *c) { ++*c; }
void disk_busy(struct disk *);
void disk_unbusy(struct disk *, size_t, int);
int disk_ioctl(struct disk *, dev_t, unsigned long, void *, int, lwp_t *);
void biodone(struct buf *);
zvol_state_t *zvol_os_hold(dev_t, boolean_t);
void zvol_os_rele(zvol_state_t *);
int zvol_first_open(zvol_state_t *, boolean_t);
void zvol_last_close(zvol_state_t *);
zilog_t *zil_open(objset_t *, void *, int *);
int zil_commit(zilog_t *, uint64_t);
#define zvol_get_data NULL
zfs_locked_range_t *zfs_rangelock_enter(int *, uint64_t, uint64_t, int);
void zfs_rangelock_exit(zfs_locked_range_t *);
int dmu_read_uio_dnode(dnode_t *, zfs_uio_t *, uint64_t, int);
dmu_tx_t *dmu_tx_create(objset_t *);
void dmu_tx_hold_write_by_dnode(dmu_tx_t *, dnode_t *, uint64_t, uint64_t);
int dmu_tx_assign(dmu_tx_t *, int);
void dmu_tx_abort(dmu_tx_t *);
int dmu_write_uio_dnode(dnode_t *, zfs_uio_t *, uint64_t, dmu_tx_t *, int);
void zvol_log_write(zvol_state_t *, dmu_tx_t *, uint64_t, uint64_t, boolean_t);
void dmu_tx_commit(dmu_tx_t *);
void dataset_kstats_update_read_kstats(dataset_kstats_t *, int64_t);
void dataset_kstats_update_write_kstats(dataset_kstats_t *, int64_t);
int zvol_open(dev_t, int, int, lwp_t *);
int zvol_close(dev_t, int, int, lwp_t *);
int zvol_read(dev_t, struct uio *, int);
int zvol_write(dev_t, struct uio *, int);
int zvol_ioctl(dev_t, unsigned long, void *, int, lwp_t *);
void zvol_strategy(struct buf *);
#endif

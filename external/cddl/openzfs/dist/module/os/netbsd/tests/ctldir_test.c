/* SPDX-License-Identifier: CDDL-1.0 */
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define ZFS_MAX_DATASET_NAME_LEN 256
#define ZFSCTL_INO_ROOT 1
#define ZFSCTL_INO_SNAPDIR 2
#define ZFS_SNAPDIR_NAME "snapshot"
#define KM_SLEEP 0
#define M_TEMP 0
#define M_WAITOK 0
#define FTAG NULL
#define DT_DIR 4
#define ASSERT3S(a, op, b) assert((a) op (b))
struct sfs_node {
	struct { uint64_t parent_id, id; } key;
};
typedef struct {
	void *z_os;
	uint64_t z_root;
} zfsvfs_t;
struct mount { zfsvfs_t *mnt_data; };
struct vnode { struct sfs_node *v_data; struct mount *v_mount; };
#define VTOSFS(vp) ((vp)->v_data)
struct uio { ssize_t uio_resid; off_t uio_offset; char *buffer; };
/* Native dirent layout and alignment. */
struct dirent {
	uint64_t d_fileno;
	uint16_t d_reclen;
	uint16_t d_namlen;
	uint8_t d_type;
	char d_name[512];
};
#define _DIRENT_SIZE(de) \
	((offsetof(struct dirent, d_name) + (de)->d_namlen + 1 + 7) & ~7)
#define _DIRENT_MINSIZE(de) \
	((offsetof(struct dirent, d_name) + 1 + 7) & ~7)
struct vop_readdir_args {
	struct vnode *a_vp;
	struct uio *a_uio;
	int *a_eofflag;
	int *a_ncookies;
	off_t **a_cookies;
};
static int enters, config, inject_error, copies, fail_copy;
static int zfs_enter(zfsvfs_t *fs, void *tag) { enters++; return 0; }
static void zfs_exit(zfsvfs_t *fs, void *tag) { assert(--enters == 0); }
static void *dmu_objset_pool(void *os) { return os; }
static void dsl_pool_config_enter(void *p, void *tag) { config++; }
static void dsl_pool_config_exit(void *p, void *tag) { config--; }
static int
dmu_snapshot_list_next(void *os, size_t len, char *name, uint64_t *id,
    uint64_t *cookie, void *conflict)
{
	assert(config == 1);
	if (inject_error)
		return inject_error;
	if (*cookie == 0) {
		strcpy(name, "first");
		*id = 100;
		*cookie = 0x80000000;
	} else if (*cookie == 0x80000000) {
		memset(name, 'x', 255);
		name[255] = '\0';
		*id = 101;
		*cookie = 0x100000001;
	} else {
		assert(*cookie == 0x100000001);
		return ENOENT;
	}
	return 0;
}
static void *kmem_zalloc(size_t n, int flags) { return calloc(1, n); }
static void kmem_free(void *p, size_t n) { free(p); }
#define strlcpy test_strlcpy
static size_t test_strlcpy(char *d, const char *s, size_t n)
{
	assert(strlen(s) < n);
	strcpy(d, s);
	return strlen(s);
}
static int uiomove(void *p, size_t n, struct uio *uio)
{
	if (++copies == fail_copy)
		return EFAULT;
	assert((ssize_t)n <= uio->uio_resid);
	memcpy(uio->buffer, p, n);
	uio->buffer += n;
	uio->uio_resid -= n;
	uio->uio_offset += n;
	return 0;
}
#define malloc(n, type, flags) malloc(n)
#define free(p, type) free(p)
#include "readdir.h"
#undef malloc
#undef free

static void
read_entries(struct vnode *vp, off_t start, size_t len, int want_error,
    int want_count, off_t end, int eof)
{
	char *buffer = malloc(len);
	struct uio uio = { len, start, buffer };
	int count = -1, ended = -1;
	off_t *cookies = NULL;
	struct vop_readdir_args ap = { vp, &uio, &ended, &count, &cookies };

	assert(sfs_readdir(&ap) == want_error);
	assert(count == want_count);
	assert(uio.uio_offset == end);
	assert(ended == eof);
	assert(enters == 0 && config == 0);
	if (count) {
		assert(cookies[count - 1] == end);
		/* Records contain zero padding and fit the output buffer. */
		size_t used = 0;
		for (int i = 0; i < count; i++) {
			struct dirent *de = (void *)(buffer + used);
			assert(de->d_type == DT_DIR);
			assert(de->d_name[de->d_namlen] == '\0');
			for (size_t j = offsetof(struct dirent, d_name) +
			    de->d_namlen + 1; j < de->d_reclen; j++)
				assert(((char *)de)[j] == 0);
			used += de->d_reclen;
		}
		assert(used == len - uio.uio_resid);
	}
	free(cookies);
	free(buffer);
}

int main(void)
{
	zfsvfs_t fs = { NULL, 42 };
	struct mount mp = { &fs };
	struct sfs_node node = { { 0, ZFSCTL_INO_ROOT } };
	struct vnode vp = { &node, &mp };

	read_entries(&vp, 0, 1024, 0, 3, 3, 1);
	read_entries(&vp, 3, 1024, 0, 0, 3, 1);
	read_entries(&vp, 0, 1, EINVAL, 0, 0, 0);
	read_entries(&vp, 0, 16, 0, 1, 1, 0);
	read_entries(&vp, 1, 15, EINVAL, 0, 1, 0);
	node.key.parent_id = ZFSCTL_INO_ROOT;
	node.key.id = ZFSCTL_INO_SNAPDIR;
	read_entries(&vp, 0, 1024, 0, 4, 0x100000003, 1);
	read_entries(&vp, 2, 24, 0, 1, 0x80000002, 0);
	read_entries(&vp, 0x80000002, 272, 0, 1, 0x100000003, 0);
	read_entries(&vp, 0x100000003, 1024, 0, 0, 0x100000003, 1);
	inject_error = EIO;
	read_entries(&vp, 2, 1024, EIO, 0, 2, 0);
	inject_error = 0;
	fail_copy = copies + 1;
	read_entries(&vp, 2, 1024, EFAULT, 0, 2, 0);
	puts("control-directory readdir tests passed");
	return 0;
}

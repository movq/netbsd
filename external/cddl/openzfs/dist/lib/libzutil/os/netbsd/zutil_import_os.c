/* SPDX-License-Identifier: BSD-2-Clause */
#include <sys/types.h>
#include <sys/disk.h>
#include <sys/dkio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <sys/vdev_impl.h>
#include <libzutil.h>
#include "zutil_import.h"

void
zpool_open_func(void *arg)
{
	rdsk_node_t *rn = arg;
	struct stat st;
	struct dkwedge_list wedges = { 0 };
	nvlist_t *config;
	int labels, fd;

	/* Like osnet, enumerate block devices and read through their raw nodes. */
	if (stat(rn->rn_name, &st) == -1 ||
	    (!S_ISBLK(st.st_mode) && !S_ISREG(st.st_mode)))
		return;
	if (S_ISBLK(st.st_mode)) {
		char raw[MAXPATHLEN];
		fd = opendisk(rn->rn_name, O_RDONLY | O_NONBLOCK | O_CLOEXEC,
		    raw, sizeof (raw), 0);
		if (fd == -1)
			fd = open(rn->rn_name, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	} else {
		fd = open(rn->rn_name, O_RDONLY | O_CLOEXEC);
	}
	if (fd == -1)
		return;
	if (fstat64_blk(fd, &st) != 0 || st.st_size < SPA_MINDEVSIZE)
		goto out;
	/* Do not import a parent disk when its wedges carry the labels. */
	if (!S_ISREG(st.st_mode) &&
	    ioctl(fd, DIOCLWEDGES, &wedges) == 0 && wedges.dkwl_nwedges != 0)
		goto out;
	if (zpool_read_label(fd, &config, &labels) != 0)
		goto out;
	if (labels == 0) {
		nvlist_free(config);
		goto out;
	}
	rn->rn_config = config;
	rn->rn_num_labels = labels;
out:
	close(fd);
}

const char * const *
zpool_default_search_paths(size_t *count)
{
	static const char * const paths[] = { "/dev" };
	*count = __arraycount(paths);
	return (paths);
}

int
zpool_find_import_blkid(libpc_handle_t *hdl, pthread_mutex_t *lock,
    avl_tree_t **cache)
{
	size_t count;
	const char * const *paths = zpool_default_search_paths(&count);
	return (zpool_find_import_scan(hdl, lock, cache, paths, count));
}

int
zfs_dev_flush(int fd)
{
	int force = 1;
	return (ioctl(fd, DIOCCACHESYNC, &force));
}

void
update_vdev_config_dev_strs(nvlist_t *nv)
{
	(void) nvlist_remove_all(nv, ZPOOL_CONFIG_DEVID);
	(void) nvlist_remove_all(nv, ZPOOL_CONFIG_PHYS_PATH);
}

void
update_vdev_config_dev_sysfs_path(nvlist_t *nv, const char *path,
    const char *key)
{
	(void) nv;
	(void) path;
	(void) key;
}

void
update_vdevs_config_dev_sysfs_path(nvlist_t *config)
{
	(void) config;
}

int
zpool_disk_wait(const char *path)
{
	(void) path;
	return (ENOTSUP);
}

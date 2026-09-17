/* SPDX-License-Identifier: BSD-2-Clause */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/zfs_ioctl.h>
#include <sys/zfs_ioctl_os.h>
#include "libzfs_core_impl.h"

__CTASSERT(sizeof (zfs_iocparm_t) == 24);
__CTASSERT(offsetof(zfs_iocparm_t, zfs_cmd) == 8);

int
lzc_ioctl_fd_os(int fd, unsigned long request, zfs_cmd_t *zc)
{
	zfs_iocparm_t zp = {
		.zfs_ioctl_version = ZFS_IOCVER_OZFS,
		.zfs_cmd = (uint64_t)(uintptr_t)zc,
		.zfs_cmd_size = sizeof (*zc)
	};

	return (ioctl(fd, ZFS_IOC_OS(request), &zp));
}

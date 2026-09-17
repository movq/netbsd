/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_ZFS_IOCTL_OS_H_
#define	_NETBSD_ZFS_IOCTL_OS_H_

#include <sys/types.h>
#include <sys/ioccom.h>

/*
 * Only the OpenZFS command layout is supported. The indirect argument lets
 * the driver return zfs_cmd_t (notably nvlist sizes) even on an ioctl error.
 * Explicit padding keeps the envelope identical for 32- and 64-bit callers.
 */
#define	ZFS_IOCVER_OZFS	15
typedef struct zfs_iocparm {
	uint32_t	zfs_ioctl_version;
	uint32_t	zfs_pad;
	uint64_t	zfs_cmd;
	uint64_t	zfs_cmd_size;
} zfs_iocparm_t;

#define	ZFS_IOCREQ(ioc)	((ioc) & 0xff)
#define	ZFS_IOC_OS(ioc)	_IOWR('Z', ZFS_IOCREQ(ioc), zfs_iocparm_t)

#ifdef _KERNEL
extern const struct fileops zfsdev_fileops;
extern int zfs_bmajor;
extern int zfs_cmajor;
int zfsdev_busy(void);
#endif

#endif

/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _LIBSPL_NETBSD_MOUNT_H_
#define	_LIBSPL_NETBSD_MOUNT_H_
#include_next <sys/mount.h>

#define	MS_RDONLY	MNT_RDONLY
#define	MS_NOSUID	MNT_NOSUID
#define	MS_NODEV	MNT_NODEV
#define	MS_NOEXEC	MNT_NOEXEC
#define	MS_NOATIME	MNT_NOATIME
#define	MS_SYNCHRONOUS	MNT_SYNCHRONOUS
#define	MS_REMOUNT	MNT_UPDATE
#define	MS_FORCE	MNT_FORCE
#define	MS_NOMNTTAB	0
/* Private libzfs flags; never pass these to mount or unmount. */
#define	MS_OVERLAY	0x20000000
#define	MS_CRYPT	0x40000000
#define	MS_DETACH	0x10000000
#define	umount2(p, f)	unmount(p, f)

/* ABI retained by the native kernel adapter from osnet/sys/sys/mount.h. */
struct zfs_args {
	char fspec[255];
	char dataptr[MAXPATHLEN];
	char optptr[MAXPATHLEN];
	char *fstype;
	int mflag;
	int datalen;
	int optlen;
	int flags;
};
#endif

/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_USERLAND_ZFS_CONFIG_H_
#define	_NETBSD_USERLAND_ZFS_CONFIG_H_

#define	ZFS_META_NAME		"zfs"
#define	ZFS_META_VERSION	"2.4.4"
#define	ZFS_META_RELEASE	"NetBSD"
#define	ZFS_META_ALIAS		ZFS_META_NAME "-" ZFS_META_VERSION "-" ZFS_META_RELEASE
#define	ZFS_META_LICENSE	"CDDL"
#define	ZFS_META_COPYRIGHT	"OpenZFS"
#define	HAVE_ISSETUGID		1
#define	HAVE_STRLCPY		1
#define	HAVE_STRLCAT		1
#define	HAVE_GETTEXT		1
#define	HAVE_GETRANDOM		1
#define	HAVE_MLOCKALL		1
#define	HAVE_PTHREAD_SETNAME_NP	1
#define	HAVE_ZLIB		1
#define	HAVE_XDR_BYTESREC	1

#define	SBINDIR		"/sbin"
#define	ZFSEXECDIR	"/usr/libexec/zfs"
#define	SYSCONFDIR	"/etc"
#define	PKGDATADIR	"/usr/share/zfs"
#define	ZPOOL_SCRIPTS_DIR	"/etc/zfs/zpool.d"
#define	ZPOOL_COMPAT_SYSCONF_DIR	"/etc/zfs/compatibility.d"
#define	ZPOOL_COMPAT_DATA_DIR	"/usr/share/zfs/compatibility.d"

#endif

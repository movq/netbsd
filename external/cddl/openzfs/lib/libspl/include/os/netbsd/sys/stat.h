/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _LIBSPL_NETBSD_STAT_H_
#define	_LIBSPL_NETBSD_STAT_H_
#include_next <sys/stat.h>
#include <sys/dkio.h>
#include <sys/ioctl.h>

#define	stat64	stat
#define	fstat64	fstat
#define	lstat64	lstat
#define	MAXOFFSET_T	INT64_MAX

/* Solaris fstat reports the size of disk devices as well as regular files. */
static inline int
fstat64_blk(int fd, struct stat *st)
{
	if (fstat(fd, st) == -1)
		return (-1);
	if (S_ISBLK(st->st_mode) || S_ISCHR(st->st_mode))
		return (ioctl(fd, DIOCGMEDIASIZE, &st->st_size));
	return (0);
}
#endif

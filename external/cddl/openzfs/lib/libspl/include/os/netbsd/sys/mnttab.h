/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _LIBSPL_NETBSD_MNTTAB_H_
#define	_LIBSPL_NETBSD_MNTTAB_H_
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/mount.h>
#include <stdio.h>
#include <paths.h>

/* A seekable stream supplies the index for the getvfsstat snapshot. */
#define	MNTTAB	_PATH_DEVZERO
#define	MNT_LINE_MAX	4108

struct mnttab {
	char *mnt_special;
	char *mnt_mountp;
	char *mnt_fstype;
	char *mnt_mntopts;
};
struct extmnttab {
	char *mnt_special;
	char *mnt_mountp;
	char *mnt_fstype;
	char *mnt_mntopts;
	uint_t mnt_major;
	uint_t mnt_minor;
};
int getmntent(FILE *, struct mnttab *);
int getmntany(FILE *, struct mnttab *, struct mnttab *);
int getextmntent(const char *, struct extmnttab *, struct stat *);
char *hasmntopt(struct mnttab *, const char *);
void statvfs2mnttab(struct statvfs *, struct mnttab *);
#endif

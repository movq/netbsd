/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * NetBSD exports(5) adapter. Use the same exports file and mountd reload
 * mechanism as osnet, with the common OpenZFS export-file locking.
 */
#include <sys/types.h>
#include <sys/param.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libshare.h>
#include "nfs.h"

#define	ZFS_EXPORTS_FILE	"/etc/zfs/exports"
#define	ZFS_EXPORTS_LOCK	ZFS_EXPORTS_FILE ".lock"

static int
write_share(sa_share_impl_t share, FILE *fp)
{
	static const char * const keywords[] = {
		"ro", "maproot", "mapall", "mask", "network", "alldirs",
		"public", "webnfs", "index", "quiet", "sec"
	};
	char *mountpoint, *options, *set, *next;
	boolean_t allocated;
	int error = nfs_escape_mountpoint(share->sa_mountpoint, &mountpoint,
	    &allocated);
	if (error != SA_OK)
		return (error);
	options = strdup(strcmp(share->sa_shareopts, "on") == 0 ?
	    "" : share->sa_shareopts);
	if (options == NULL) {
		error = SA_NO_MEMORY;
		goto out;
	}
	next = options;
	while ((set = strsep(&next, ";")) != NULL) {
		char *token;
		fprintf(fp, "%s\t", mountpoint);
		while ((token = strsep(&set, ", \t")) != NULL) {
			if (*token == '-')
				token++;
			if (*token == '\0')
				continue;
			size_t len = strcspn(token, "=");
			for (size_t i = 0; i < __arraycount(keywords); i++) {
				if (strlen(keywords[i]) == len &&
				    strncmp(token, keywords[i], len) == 0) {
					fputc('-', fp);
					break;
				}
			}
			fprintf(fp, "%s ", token);
		}
		fputc('\n', fp);
	}
	free(options);
	if (ferror(fp))
		error = SA_SYSTEM_ERR;
out:
	if (allocated)
		free(mountpoint);
	return (error);
}

static int
remove_share(sa_share_impl_t share, FILE *fp)
{
	(void) share, (void) fp;
	return (SA_OK);
}

static int
nfs_enable(sa_share_impl_t share)
{
	return (nfs_toggle_share(ZFS_EXPORTS_LOCK, ZFS_EXPORTS_FILE,
	    "/etc/zfs", share, write_share));
}

static int
nfs_disable(sa_share_impl_t share)
{
	return (nfs_toggle_share(ZFS_EXPORTS_LOCK, ZFS_EXPORTS_FILE,
	    "/etc/zfs", share, remove_share));
}

static boolean_t
nfs_shared(sa_share_impl_t share)
{
	return (nfs_is_shared_impl(ZFS_EXPORTS_FILE, share));
}

static int
nfs_validate(const char *opts)
{
	return (*opts == '\0' ? SA_SYNTAX_ERR : SA_OK);
}

static int
nfs_commit(void)
{
	FILE *fp = fopen("/var/run/mountd.pid", "re");
	long pid;
	int count;

	if (fp == NULL)
		return (errno == ENOENT ? SA_OK : SA_SYSTEM_ERR);
	count = fscanf(fp, "%ld", &pid);
	fclose(fp);
	if (count != 1 || pid <= 1 || pid > INT_MAX)
		return (SA_SYSTEM_ERR);
	if (kill((pid_t)pid, SIGHUP) == -1 && errno != ESRCH)
		return (SA_SYSTEM_ERR);
	return (SA_OK);
}

static void
nfs_truncate(void)
{
	nfs_reset_shares(ZFS_EXPORTS_LOCK, ZFS_EXPORTS_FILE);
}

const sa_fstype_t libshare_nfs_type = {
	.enable_share = nfs_enable,
	.disable_share = nfs_disable,
	.is_shared = nfs_shared,
	.validate_shareopts = nfs_validate,
	.commit_shares = nfs_commit,
	.truncate_shares = nfs_truncate
};

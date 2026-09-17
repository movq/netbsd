/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Solaris mount-table interfaces using NetBSD's getvfsstat, as in osnet.
 * Returned strings live until the next mount-table call on this thread.
 */
#include <sys/mnttab.h>
#include <sys/mntent.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct mount_cache {
	struct statvfs *mounts;
	struct statvfs path;
	int count;
	char opts[256];
};
static pthread_key_t cache_key;
static pthread_once_t cache_once = PTHREAD_ONCE_INIT;
static int cache_error;

static void
cache_free(void *arg)
{
	struct mount_cache *cache = arg;
	free(cache->mounts);
	free(cache);
}

static void
cache_init(void)
{
	cache_error = pthread_key_create(&cache_key, cache_free);
}

static struct mount_cache *
get_cache(void)
{
	struct mount_cache *cache;
	pthread_once(&cache_once, cache_init);
	if (cache_error != 0) {
		errno = cache_error;
		return (NULL);
	}
	cache = pthread_getspecific(cache_key);
	if (cache == NULL) {
		cache = calloc(1, sizeof (*cache));
		if (cache == NULL)
			return (NULL);
		int error = pthread_setspecific(cache_key, cache);
		if (error != 0) {
			free(cache);
			errno = error;
			return (NULL);
		}
	}
	return (cache);
}

static int
refresh(struct mount_cache *cache)
{
	int count = getvfsstat(NULL, 0, ST_WAIT);
	if (count == -1)
		return (-1);
	size_t size = ((size_t)count + 16) * sizeof (*cache->mounts);
	struct statvfs *mounts = malloc(size);
	if (mounts == NULL)
		return (-1);
	count = getvfsstat(mounts, size, ST_WAIT);
	if (count == -1) {
		free(mounts);
		return (-1);
	}
	free(cache->mounts);
	cache->mounts = mounts;
	cache->count = count;
	return (0);
}

void
statvfs2mnttab(struct statvfs *st, struct mnttab *mt)
{
	struct mount_cache *cache = get_cache();
	/* All callers obtained the per-thread cache before getting here. */
	if (cache == NULL)
		abort();
	snprintf(cache->opts, sizeof (cache->opts), "%s,%s,%s,%s,%s",
	    st->f_flag & ST_RDONLY ? MNTOPT_RO : MNTOPT_RW,
	    st->f_flag & ST_NOSUID ? MNTOPT_NOSETUID : MNTOPT_SETUID,
	    st->f_flag & ST_NOEXEC ? MNTOPT_NOEXEC : MNTOPT_EXEC,
	    st->f_flag & ST_NODEV ? MNTOPT_NODEVICES : MNTOPT_DEVICES,
	    st->f_flag & ST_NOATIME ? MNTOPT_NOATIME : MNTOPT_ATIME);
	mt->mnt_special = st->f_mntfromname;
	mt->mnt_mountp = st->f_mntonname;
	mt->mnt_fstype = st->f_fstypename;
	mt->mnt_mntopts = cache->opts;
}

int
getmntent(FILE *fp, struct mnttab *mt)
{
	struct mount_cache *cache = get_cache();
	off_t index = lseek(fileno(fp), 0, SEEK_CUR);
	if (cache == NULL || index == -1)
		return (errno);
	if ((index == 0 || cache->mounts == NULL) && refresh(cache) == -1)
		return (errno);
	if (index >= cache->count)
		return (-1);
	statvfs2mnttab(&cache->mounts[index], mt);
	if (lseek(fileno(fp), 1, SEEK_CUR) == -1)
		return (errno);
	return (0);
}

int
getmntany(FILE *fp, struct mnttab *mt, struct mnttab *ref)
{
	struct mount_cache *cache = get_cache();
	(void) fp;
	if (cache == NULL || refresh(cache) == -1)
		return (errno);
	for (int i = 0; i < cache->count; i++) {
		struct statvfs *st = &cache->mounts[i];
		if ((ref->mnt_special != NULL &&
		    strcmp(ref->mnt_special, st->f_mntfromname) != 0) ||
		    (ref->mnt_mountp != NULL &&
		    strcmp(ref->mnt_mountp, st->f_mntonname) != 0) ||
		    (ref->mnt_fstype != NULL &&
		    strcmp(ref->mnt_fstype, st->f_fstypename) != 0))
			continue;
		statvfs2mnttab(st, mt);
		return (0);
	}
	return (-1);
}

char *
hasmntopt(struct mnttab *mt, const char *opt)
{
	size_t len = strlen(opt);
	char *p = mt->mnt_mntopts;
	while (p != NULL && *p != '\0') {
		if (strncmp(p, opt, len) == 0 &&
		    (p[len] == '\0' || p[len] == ',' || p[len] == '='))
			return (p);
		p = strchr(p, ',');
		if (p != NULL)
			p++;
	}
	return (NULL);
}

int
getextmntent(const char *path, struct extmnttab *mt, struct stat *st)
{
	struct mount_cache *cache = get_cache();
	if (cache == NULL || stat(path, st) == -1 ||
	    statvfs(path, &cache->path) == -1)
		return (-1);
	statvfs2mnttab(&cache->path, (struct mnttab *)mt);
	mt->mnt_major = major(st->st_dev);
	mt->mnt_minor = minor(st->st_dev);
	return (0);
}

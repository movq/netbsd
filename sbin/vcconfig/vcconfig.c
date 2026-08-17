/*	$NetBSD$	*/

/*
 * Copyright (c) 2026 Mike Jones <mike@mjones.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/dkio.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <dev/cgdvar.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "veracrypt.h"

static int	backing_open(const char *, char *, size_t, uint64_t *);
static void	usage(void) __dead;

static int
backing_open(const char *name, char *block_path, size_t block_path_size,
    uint64_t *sizep)
{
	char device[MAXPATHLEN], opened[MAXPATHLEN];
	const char *resolved;
	struct stat st, bst;
	off_t media_size;
	u_int sector_size;
	int fd = -1, saved_errno;

	resolved = getfsspecname(device, sizeof(device), name);
	if (resolved == NULL)
		return -1;
	fd = opendisk(resolved, O_RDONLY, opened, sizeof(opened), 0);
	if (fd == -1)
		return -1;
	if (fstat(fd, &st) == -1)
		goto fail;
	if (S_ISBLK(st.st_mode)) {
		if (strlcpy(block_path, opened, block_path_size) >=
		    block_path_size) {
			errno = ENAMETOOLONG;
			goto fail;
		}
	} else if (S_ISCHR(st.st_mode)) {
		if (getdiskcookedname(block_path, block_path_size, opened) == NULL)
			goto fail;
	} else {
		errno = ENODEV;
		goto fail;
	}
	if (stat(block_path, &bst) == -1)
		goto fail;
	if (!S_ISBLK(bst.st_mode)) {
		errno = ENOTBLK;
		goto fail;
	}
	if (ioctl(fd, DIOCGSECTORSIZE, &sector_size) == -1 ||
	    ioctl(fd, DIOCGMEDIASIZE, &media_size) == -1)
		goto fail;
	if (sector_size != DEV_BSIZE || media_size <= 0) {
		errno = EINVAL;
		goto fail;
	}
	*sizep = (uint64_t)media_size;
	return fd;

fail:
	saved_errno = errno;
	close(fd);
	errno = saved_errno;
	return -1;
}

int
main(int argc, char *argv[])
{
	struct cgd_ioctl2 ci;
	struct cgd_user cgu;
	struct vc_mapping mapping;
	uint8_t volume_key[VC_VOLUME_KEY_SIZE];
	char backing_path[MAXPATHLEN], cgd_path[MAXPATHLEN];
	char passphrase[VC_PASSWORD_MAX + 1], why[160];
	char *end;
	uint64_t device_size;
	intmax_t pim_value = 0;
	uint32_t pim = 0;
	int backing_fd = -1, cgd_fd = -1, ch, error = 1, parse_error;

	while ((ch = getopt(argc, argv, "p:")) != -1) {
		switch (ch) {
		case 'p':
			end = NULL;
			pim_value = strtoi(optarg, &end, 10, 0, VC_PIM_MAX,
			    &parse_error);
			if (parse_error != 0 || end == optarg || *end != '\0')
				errx(1, "invalid PIM: %s", optarg);
			pim = (uint32_t)pim_value;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 2)
		usage();

	memset(&ci, 0, sizeof(ci));
	memset(&cgu, 0, sizeof(cgu));
	memset(&mapping, 0, sizeof(mapping));
	memset(volume_key, 0, sizeof(volume_key));
	memset(passphrase, 0, sizeof(passphrase));

	backing_fd = backing_open(argv[1], backing_path,
	    sizeof(backing_path), &device_size);
	if (backing_fd == -1)
		err(1, "%s", argv[1]);
	cgd_fd = opendisk(argv[0], O_RDWR, cgd_path, sizeof(cgd_path), 0);
	if (cgd_fd == -1)
		err(1, "%s", argv[0]);
	cgu.cgu_unit = -1;
	if (ioctl(cgd_fd, CGDIOCGET, &cgu) == -1)
		err(1, "%s: CGDIOCGET", cgd_path);
	if (cgu.cgu_dev != 0)
		errx(1, "%s is already in use", cgd_path);

	if (getpassfd("Passphrase: ", passphrase, sizeof(passphrase), NULL,
	    GETPASS_NEED_TTY, 0) == NULL) {
		warn("could not read passphrase");
		goto out;
	}
	if (vc_read_unlock(backing_fd, device_size, passphrase,
	    strlen(passphrase), pim, &mapping, volume_key, why,
	    sizeof(why)) == -1) {
		warnx("%s", why);
		goto out;
	}
	explicit_memset(passphrase, 0, sizeof(passphrase));

	ci.ci_disk = backing_path;
	ci.ci_alg = "aes-xts";
	ci.ci_ivmethod = "encblkno1";
	ci.ci_keylen = VC_VOLUME_KEY_SIZE * NBBY;
	ci.ci_key = (const char *)volume_key;
	ci.ci_blocksize = 128;
	ci.ci_data_offset = mapping.data_offset;
	ci.ci_data_length = mapping.data_length;
	ci.ci_iv_offset = mapping.iv_offset;
	if (ioctl(cgd_fd, CGDIOCSET2, &ci) == -1) {
		warn("CGDIOCSET2");
		goto out;
	}
	error = 0;

out:
	explicit_memset(passphrase, 0, sizeof(passphrase));
	explicit_memset(volume_key, 0, sizeof(volume_key));
	explicit_memset(&ci, 0, sizeof(ci));
	explicit_memset(&cgu, 0, sizeof(cgu));
	explicit_memset(&mapping, 0, sizeof(mapping));
	if (backing_fd != -1)
		close(backing_fd);
	if (cgd_fd != -1)
		close(cgd_fd);
	return error;
}

static void
usage(void)
{
	fprintf(stderr, "usage: vcconfig [-p pim] cgd device\n");
	exit(1);
}

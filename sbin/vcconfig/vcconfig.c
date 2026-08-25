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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "veracrypt.h"

#define AES_XTS_128_KEY_SIZE	32
#define AES_XTS_256_KEY_SIZE	64

static int	backing_open(const char *, char *, size_t, uint64_t *);
static ssize_t	keyfile_read(const char *, uint8_t[VC_VOLUME_KEY_SIZE]);
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

static ssize_t
keyfile_read(const char *path, uint8_t key[VC_VOLUME_KEY_SIZE])
{
	uint8_t input[VC_VOLUME_KEY_SIZE + 1];
	size_t done;
	ssize_t n, result;
	int fd, saved_errno;

	memset(input, 0, sizeof(input));
	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd == -1)
		return -1;

	done = 0;
	while (done < sizeof(input)) {
		n = read(fd, input + done, sizeof(input) - done);
		if (n == -1) {
			if (errno == EINTR)
				continue;
			result = -1;
			goto out;
		}
		if (n == 0)
			break;
		done += (size_t)n;
	}

	if (done != AES_XTS_128_KEY_SIZE &&
	    done != AES_XTS_256_KEY_SIZE) {
		result = 0;
		goto out;
	}
	memcpy(key, input, done);
	result = (ssize_t)done;

out:
	saved_errno = errno;
	explicit_memset(input, 0, sizeof(input));
	close(fd);
	errno = saved_errno;
	return result;
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
	const char *keyfile = NULL;
	char *end;
	uint64_t data_offset, device_size, raw_offset = 0;
	uintmax_t offset_value;
	intmax_t pim_value;
	ssize_t key_size;
	uint32_t pim = 0;
	int backing_fd = -1, cgd_fd = -1, ch, error = 1, parse_error;
	bool offset_set = false, pim_set = false;

	while ((ch = getopt(argc, argv, "k:o:p:")) != -1) {
		switch (ch) {
		case 'k':
			keyfile = optarg;
			break;
		case 'o':
			end = NULL;
			offset_value = strtou(optarg, &end, 10, 0,
			    UINT64_MAX / DEV_BSIZE, &parse_error);
			if (parse_error != 0 || end == optarg || *end != '\0')
				errx(1, "invalid sector offset: %s", optarg);
			raw_offset = (uint64_t)offset_value;
			offset_set = true;
			break;
		case 'p':
			end = NULL;
			pim_value = strtoi(optarg, &end, 10, 0, VC_PIM_MAX,
			    &parse_error);
			if (parse_error != 0 || end == optarg || *end != '\0')
				errx(1, "invalid PIM: %s", optarg);
			pim = (uint32_t)pim_value;
			pim_set = true;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 2)
		usage();
	if (keyfile != NULL && !offset_set)
		errx(1, "-k requires -o");
	if (keyfile == NULL && offset_set)
		errx(1, "-o requires -k");
	if (keyfile != NULL && pim_set)
		errx(1, "-p cannot be used with -k");

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

	if (keyfile != NULL) {
		key_size = keyfile_read(keyfile, volume_key);
		if (key_size == -1) {
			warn("%s", keyfile);
			goto out;
		}
		if (key_size == 0) {
			warnx("%s must contain exactly 32 or 64 bytes", keyfile);
			goto out;
		}
		data_offset = raw_offset * DEV_BSIZE;
		if (data_offset >= device_size) {
			warnx("sector offset extends beyond %s", backing_path);
			goto out;
		}
		mapping.data_offset = data_offset;
		mapping.data_length = device_size - data_offset;
		mapping.iv_offset = raw_offset;
	} else {
		if (getpassfd("Passphrase: ", passphrase, sizeof(passphrase),
		    NULL, GETPASS_NEED_TTY, 0) == NULL) {
			warn("could not read passphrase");
			goto out;
		}
		if (vc_read_unlock(backing_fd, device_size, passphrase,
		    strlen(passphrase), pim, &mapping, volume_key, why,
		    sizeof(why)) == -1) {
			warnx("%s", why);
			goto out;
		}
		key_size = VC_VOLUME_KEY_SIZE;
	}
	explicit_memset(passphrase, 0, sizeof(passphrase));

	ci.ci_disk = backing_path;
	ci.ci_alg = "aes-xts";
	ci.ci_ivmethod = "encblkno1";
	ci.ci_keylen = (size_t)key_size * NBBY;
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
	fprintf(stderr,
	    "usage: vcconfig [-p pim] cgd device\n"
	    "       vcconfig -k keyfile -o sectors cgd device\n");
	exit(1);
}

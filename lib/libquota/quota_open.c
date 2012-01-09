/*	$NetBSD: quota_open.c,v 1.2 2012/01/09 15:27:04 dholland Exp $	*/
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David A. Holland.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: quota_open.c,v 1.2 2012/01/09 15:27:04 dholland Exp $");

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <quota.h>
#include "quotapvt.h"

struct quotahandle *
quota_open(const char *path)
{
	struct quotahandle *qh;
	int serrno;

	qh = malloc(sizeof(*qh));
	if (qh == NULL) {
		return NULL;
	}
	qh->qh_mountpoint = strdup(path);
	if (qh->qh_mountpoint == NULL) {
		serrno = errno;
		free(qh);
		errno = serrno;
		return NULL;
	}
	return qh;
}

const char *
quota_getmountpoint(struct quotahandle *qh)
{
	return qh->qh_mountpoint;
}

const char *
quota_getmountdevice(struct quotahandle *qh)
{
	errno = ENOSYS;
	return NULL;
}

void
quota_close(struct quotahandle *qh)
{
	free(qh->qh_mountpoint);
	free(qh);
}

/*	$NetBSD: sysfs.h,v 1.2 2014/03/18 18:20:43 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_SYSFS_H_
#define _LINUX_SYSFS_H_

#include <sys/param.h>
#include <sys/types.h>

#include <linux/stat.h>
#include <linux/types.h>

struct kobject;
struct file;

struct attribute {
	const char *name;
	umode_t mode;
};

struct bin_attribute {
	struct attribute attr;
	size_t size;
	void *private;
	ssize_t (*read)(struct file *, struct kobject *,
	    const struct bin_attribute *, char *, loff_t, size_t);
	ssize_t (*write)(struct file *, struct kobject *,
	    const struct bin_attribute *, char *, loff_t, size_t);
};

struct attribute_group {
	const char *name;
	struct attribute **attrs;
	struct bin_attribute **bin_attrs;
};

#define	ATTRIBUTE_GROUPS(name)

#define	sysfs_create_link(kobj, target, name)	0
#define	sysfs_remove_link(kobj, name)		do { } while (0)
#define	sysfs_create_group(kobj, grp)		0
#define	sysfs_remove_group(kobj, grp)		do { } while (0)
#define	sysfs_create_file(kobj, attr)		0
#define	sysfs_remove_file(kobj, attr)		do { } while (0)
#define	sysfs_create_bin_file(kobj, attr)	0
#define	sysfs_remove_bin_file(kobj, attr)	do { } while (0)
#define	sysfs_remove_file_from_group(kobj, attr, group) \
	do { } while (0)
#define	sysfs_create_files(kobj, attrs)		0
#define	sysfs_remove_files(kobj, attrs)		do { } while (0)
#define	sysfs_bin_attr_init(attr)		do { } while (0)
#define	sysfs_update_group(kobj, grp)		0

static inline int
sysfs_emit(char *buf, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vsnprintf(buf, PAGE_SIZE, fmt, ap);
	va_end(ap);

	return ret;
}

static inline int
sysfs_emit_at(char *buf, int at, const char *fmt, ...)
{
	va_list ap;
	int ret;

	if (at < 0 || at >= PAGE_SIZE)
		return 0;

	va_start(ap, fmt);
	ret = vsnprintf(buf + at, PAGE_SIZE - at, fmt, ap);
	va_end(ap);

	return ret;
}

#endif  /* _LINUX_SYSFS_H_ */

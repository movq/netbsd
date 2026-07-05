/*	$NetBSD: device.h,v 1.17 2022/07/29 23:50:44 riastradh Exp $	*/

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

#ifndef _LINUX_DEVICE_H_
#define _LINUX_DEVICE_H_

#include <sys/types.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <linux/hrtimer.h>
#include <linux/ratelimit.h>
#include <linux/sysfs.h>

#define	dev_get_drvdata		linux_dev_get_drvdata
#define	dev_set_drvdata		linux_dev_set_drvdata

struct device_attribute {
	struct attribute attr;
	ssize_t (*show)(struct device *, struct device_attribute *, char *);
	ssize_t (*store)(struct device *, struct device_attribute *,
	    const char *, size_t);
};

#define	__ATTR(_name, _mode, _show, _store) {			      \
	.attr = { .name = #_name, .mode = (_mode) },		      \
	.show = (_show),						      \
	.store = (_store),					      \
}

#define	DEVICE_ATTR(_name, _mode, _show, _store)			      \
	struct device_attribute dev_attr_##_name =			      \
	    __ATTR(_name, _mode, _show, _store)
#define	DEVICE_ATTR_RO(_name)					      \
	DEVICE_ATTR(_name, 0444, _name##_show, NULL)
#define	DEVICE_ATTR_RW(_name)					      \
	DEVICE_ATTR(_name, 0644, _name##_show, _name##_store)

#define	device_create_file(dev, attr)		\
	((void)(dev), (void)(attr), 0)
#define	device_remove_file(dev, attr)		do {	\
	(void)(dev);					\
	(void)(attr);					\
} while (0)

void	linux_device_init(void);
void	linux_device_fini(void);

void *	dev_get_drvdata(struct device *);
void	dev_set_drvdata(struct device *, void *);

#define	dev_crit(DEV, FMT, ...)	do {					      \
	if (DEV)							      \
		aprint_error_dev((DEV), "critical: " FMT, ##__VA_ARGS__);     \
	else								      \
		aprint_error("critical: " FMT, ##__VA_ARGS__);		      \
} while (0)

#define	dev_emerg(DEV, FMT, ...)	do {					      \
	if (DEV)							      \
		aprint_error_dev((DEV), "emergency: " FMT, ##__VA_ARGS__);    \
	else								      \
		aprint_error("emergency: " FMT, ##__VA_ARGS__);		      \
} while (0)

#define	dev_err(DEV, FMT, ...)	do {					      \
	if (DEV)							      \
		aprint_error_dev((DEV), "error: " FMT, ##__VA_ARGS__);	      \
	else								      \
		aprint_error("error: " FMT, ##__VA_ARGS__);		      \
} while (0)

#define	dev_err_once	dev_err	/* XXX rate-limit */

#define	dev_err_probe(DEV, ERR, FMT, ...)	({			\
	dev_err((DEV), FMT, ##__VA_ARGS__);				\
	(ERR);								\
})

#define	dev_warn(DEV, FMT, ...)	do {					      \
	if (DEV)							      \
		aprint_normal_dev((DEV), "warn: " FMT, ##__VA_ARGS__);	      \
	else								      \
		aprint_normal("warn: " FMT, ##__VA_ARGS__);		      \
} while (0)
#define	dev_warn_once	dev_warn	/* XXX rate-limit */
#define	dev_WARN_ONCE(DEV, CONDITION, FMT, ...)	do {		      \
	if (CONDITION)							      \
		dev_warn_once((DEV), FMT, ##__VA_ARGS__);		      \
} while (0)
#define	dev_WARN	dev_warn
#define	dev_info_once	dev_info	/* XXX once */
#define	dev_dbg_once	dev_dbg		/* XXX once */

#define	dev_notice(DEV, FMT, ...)	do {				      \
	if (DEV)							      \
		aprint_normal_dev((DEV), "notice: " FMT, ##__VA_ARGS__);      \
	else								      \
		aprint_normal("notice: " FMT, ##__VA_ARGS__);		      \
} while (0)

#define	dev_info(DEV, FMT, ...)	do {					      \
	if (DEV)							      \
		aprint_normal_dev((DEV), FMT, ##__VA_ARGS__);		      \
	else								      \
		aprint_normal(FMT, ##__VA_ARGS__);			      \
} while (0)

#define	dev_dbg(DEV, FMT, ...)	do {					      \
	if (DEV)							      \
		aprint_debug_dev((DEV), "debug: " FMT, ##__VA_ARGS__);	      \
	else								      \
		aprint_debug("debug: " FMT, ##__VA_ARGS__);		      \
} while (0)

#define	dev_name	device_xname
#define	dev_is_removable(dev)	false

static inline struct device *
get_device(struct device *dev)
{

	return dev;
}

#define	put_device(x)	do { } while (0)

#define	NUMA_NO_NODE	(-1)

static inline int
dev_to_node(struct device *dev)
{

	return NUMA_NO_NODE;
}

static inline void
set_dev_node(struct device *dev, int node)
{
}

static inline const char *
dev_driver_string(struct device *dev)
{
	return device_cfdriver(dev)->cd_name;
}

#define	DPM_FLAG_NEVER_SKIP	0

#define	dev_warn_ratelimited	dev_warn
#define	dev_notice_ratelimited	dev_notice
#define	dev_err_ratelimited	dev_err
#define	dev_dbg_ratelimited	dev_dbg

static inline void
dev_pm_set_driver_flags(struct device *dev, uint32_t flags)
{
}

#endif  /* _LINUX_DEVICE_H_ */

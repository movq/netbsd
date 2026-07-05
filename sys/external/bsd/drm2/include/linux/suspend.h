/*	$NetBSD: suspend.h,v 1.4 2022/10/25 23:37:24 riastradh Exp $	*/

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

#ifndef _LINUX_SUSPEND_H_
#define _LINUX_SUSPEND_H_

#include <sys/cdefs.h>

#include <linux/notifier.h>

typedef int suspend_state_t;

#define	PM_SUSPEND_ON		0
#define	PM_SUSPEND_MEM		1
#define	PM_SUSPEND_TO_IDLE	2
#define	pm_suspend_target_state	PM_SUSPEND_MEM

enum {
	PM_HIBERNATION_PREPARE,
	PM_POST_HIBERNATION,
	PM_SUSPEND_PREPARE,
	PM_POST_SUSPEND,
};

#define	ksys_sync_helper()	__nothing

static inline int
register_pm_notifier(struct notifier_block *notifier)
{

	return 0;
}

static inline int
unregister_pm_notifier(struct notifier_block *notifier)
{

	return 0;
}

static inline bool
pm_resume_via_firmware(void)
{

	return true;
}

#endif  /* _LINUX_SUSPEND_H_ */

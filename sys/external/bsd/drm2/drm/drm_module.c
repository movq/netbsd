/*	$NetBSD: drm_module.c,v 1.32 2023/09/05 20:15:10 riastradh Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_module.c,v 1.32 2023/09/05 20:15:10 riastradh Exp $");

#include <sys/types.h>
#include <sys/condvar.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/once.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <linux/mutex.h>
#include <linux/srcu.h>
#include <linux/xarray.h>

#include <drm/drm_bridge.h>
#include <drm/drm_buddy.h>
#include <drm/drm_device.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>
#include <drm/drm_sysctl.h>

#include "../dist/drm/drm_crtc_internal.h"
#include "../dist/drm/drm_internal.h"

/*
 * XXX I2C stuff should be moved to a separate drmkms_i2c module.
 */
MODULE(MODULE_CLASS_DRIVER, drmkms, "drmkms_linux,sysmon_power");

struct mutex	drm_global_mutex;

struct drm_sysctl_def drm_def = DRM_SYSCTL_INIT();

extern struct srcu_struct drm_unplug_srcu;

static int
drm_init(void)
{
	extern int linux_guarantee_initialized(void);
	int error;

	error = linux_guarantee_initialized();
	if (error)
		return error;

	error = -drm_buddy_module_init(); /* XXX errno Linux->NetBSD */
	if (error)
		return error;

	extern bool drm_core_init_complete;
	drm_core_init_complete = true;

	if (ISSET(boothowto, AB_DEBUG))
		__drm_debug = DRM_UT_DRIVER;

	xa_init_flags(&drm_minors_xa, XA_FLAGS_ALLOC);
	_init_srcu_struct(&drm_unplug_srcu, "drmunplg");
	linux_mutex_init(&drm_global_mutex);
	linux_mutex_init(&drm_kernel_fb_helper_lock);
	drm_connector_ida_init();
	drm_panel_init_lock();
	drm_bridge_init_lock();
	drm_sysctl_init(&drm_def);

	return 0;
}

int
drm_guarantee_initialized(void)
{
#ifdef _MODULE
	return 0;
#else
	static ONCE_DECL(drm_init_once);

	return RUN_ONCE(&drm_init_once, &drm_init);
#endif
}

static void
drm_fini(void)
{

	drm_sysctl_fini(&drm_def);
	drm_bridge_fini_lock();
	drm_panel_fini_lock();
	drm_connector_ida_destroy();
	linux_mutex_destroy(&drm_kernel_fb_helper_lock);
	linux_mutex_destroy(&drm_global_mutex);
	cleanup_srcu_struct(&drm_unplug_srcu);
	xa_destroy(&drm_minors_xa);
	drm_buddy_module_exit();
}

static int
drmkms_modcmd(modcmd_t cmd, void *arg __unused)
{
#ifdef _MODULE
	devmajor_t bmajor = NODEVMAJOR, cmajor = NODEVMAJOR;
#endif
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = drm_init();
#else
		error = drm_guarantee_initialized();
#endif
		if (error)
			return error;
#ifdef _MODULE
		error = devsw_attach("drm", NULL, &bmajor,
		    &drm_cdevsw, &cmajor);
		if (error) {
			aprint_error("drmkms: unable to attach devsw: %d\n",
			    error);
			return error;
		}
#endif
		return 0;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		devsw_detach(NULL, &drm_cdevsw);
#endif
		drm_fini();
		return 0;

	default:
		return ENOTTY;
	}
}

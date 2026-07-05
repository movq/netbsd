/*	$NetBSD: linux_dmi.c,v 1.2 2018/08/27 06:45:44 riastradh Exp $	*/

/*-
 * Copyright (C) 2014 NONAKA Kimihiro <nonaka@netbsd.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_dmi.c,v 1.2 2018/08/27 06:45:44 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/pmf.h>

#include <linux/dmi.h>

const char *
dmi_get_system_info(enum dmi_field slot)
{
	switch (slot) {
	case DMI_NONE:
		return NULL;
	case DMI_BIOS_VENDOR:
		return pmf_get_platform("bios-vendor");
	case DMI_BIOS_VERSION:
		return pmf_get_platform("bios-version");
	case DMI_BIOS_DATE:
		return pmf_get_platform("bios-date");
	case DMI_SYS_VENDOR:
		return pmf_get_platform("system-vendor");
	case DMI_PRODUCT_NAME:
		return pmf_get_platform("system-product");
	case DMI_PRODUCT_VERSION:
		return pmf_get_platform("system-version");
	case DMI_PRODUCT_SERIAL:
		return pmf_get_platform("system-serial");
	case DMI_PRODUCT_UUID:
		return pmf_get_platform("system-uuid");
	case DMI_BOARD_VENDOR:
		return pmf_get_platform("board-vendor");
	case DMI_BOARD_NAME:
		return pmf_get_platform("board-product");
	case DMI_BOARD_VERSION:
		return pmf_get_platform("board-version");
	case DMI_BOARD_SERIAL:
		return pmf_get_platform("board-serial");
	case DMI_BOARD_ASSET_TAG:
		return pmf_get_platform("board-asset-tag");
	case DMI_CHASSIS_VENDOR:
		return pmf_get_platform("chassis-vendor");
	case DMI_CHASSIS_TYPE:
		return pmf_get_platform("chassis-type");
	case DMI_CHASSIS_VERSION:
		return pmf_get_platform("chassis-version");
	case DMI_CHASSIS_SERIAL:
		return pmf_get_platform("chassis-serial");
	case DMI_CHASSIS_ASSET_TAG:
		return pmf_get_platform("chassis-asset-tag");
	case DMI_STRING_MAX:
	default:
		aprint_error("%s: unknown DMI field(%d)\n", __func__, slot);
		return NULL;
	}
}

bool
dmi_match(enum dmi_field slot, const char *text)
{
	const char *p;

	if (slot == DMI_NONE) {
		aprint_error("%s: dmi_match on none makes no sense", __func__);
		return false;
	}

	p = dmi_get_system_info(slot);
	return p != NULL && strcmp(p, text) == 0;
}

static bool
dmi_found(const struct dmi_system_id *dsi)
{
	int i;

	for (i = 0; i < __arraycount(dsi->matches); i++) {
		if (dsi->matches[i].slot == DMI_NONE)
			break;
		if (!dmi_match(dsi->matches[i].slot, dsi->matches[i].substr))
			return false;
	}
	return true;
}

const struct dmi_system_id *
dmi_first_match(const struct dmi_system_id *sysid)
{
	const struct dmi_system_id *dsi;

	for (dsi = sysid; dsi->matches[0].slot != DMI_NONE; dsi++) {
		if (dmi_found(dsi))
			return dsi;
	}
	return NULL;
}

int
dmi_check_system(const struct dmi_system_id *sysid)
{
	const struct dmi_system_id *dsi;
	int num = 0;

	for (dsi = sysid; dsi->matches[0].slot != DMI_NONE; dsi++) {
		if (dmi_found(dsi)) {
			num++;
			if (dsi->callback && dsi->callback(dsi))
				break;
		}
	}
	return num;
}

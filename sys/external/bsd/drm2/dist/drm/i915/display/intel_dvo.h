/*	$NetBSD: intel_dvo.h,v 1.2 2021/12/18 23:45:30 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2019 Intel Corporation
 */

#ifndef __INTEL_DVO_H__
#define __INTEL_DVO_H__

struct intel_display;

#ifdef I915
void intel_dvo_init(struct intel_display *display);
#else
static inline void intel_dvo_init(struct intel_display *display)
{
}
#endif

#endif /* __INTEL_DVO_H__ */

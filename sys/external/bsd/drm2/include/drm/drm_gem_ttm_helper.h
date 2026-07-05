/*	$NetBSD$	*/

/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _DRM_GEM_TTM_HELPER_H_
#define _DRM_GEM_TTM_HELPER_H_

#include <drm/drm_gem.h>

struct iosys_map;

#ifdef __NetBSD__
int drm_gem_ttm_mmap(struct drm_gem_object *, off_t *, size_t, int,
    int *, int *, struct uvm_object **, int *);
#else
int drm_gem_ttm_mmap(struct drm_gem_object *, struct vm_area_struct *);
#endif
int drm_gem_ttm_vmap(struct drm_gem_object *, struct iosys_map *);
void drm_gem_ttm_vunmap(struct drm_gem_object *, struct iosys_map *);

#endif /* _DRM_GEM_TTM_HELPER_H_ */

/*	$NetBSD$	*/

/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/errno.h>

#include <uvm/uvm_extern.h>

#include <drm/drm_gem_ttm_helper.h>
#include <drm/drm_vma_manager.h>
#include <drm/ttm/ttm_bo.h>

int
drm_gem_ttm_mmap(struct drm_gem_object *obj, off_t *offp, size_t size,
    int prot, int *flagsp, int *advicep, struct uvm_object **uobjp,
    int *maxprotp)
{
	const off_t node_offset = drm_vma_node_start(&obj->vma_node);
	off_t offset;

	(void)flagsp;

	if (*offp < node_offset)
		return -EINVAL;
	offset = *offp - node_offset;
	if (offset < 0 || size > obj->size || offset > obj->size - size)
		return -EINVAL;
	if (obj->funcs == NULL || obj->funcs->vm_ops == NULL)
		return -EINVAL;

	if (obj->gemo_uvmobj.pgops == NULL)
		obj->gemo_uvmobj.pgops = obj->funcs->vm_ops;
	else
		KASSERT(obj->gemo_uvmobj.pgops == obj->funcs->vm_ops);

	*offp = offset;
	*advicep = UVM_ADV_RANDOM;
	*uobjp = &obj->gemo_uvmobj;
	*maxprotp = prot;
	return 0;
}

int
drm_gem_ttm_vmap(struct drm_gem_object *obj, struct iosys_map *map)
{
	struct ttm_buffer_object *bo =
	    container_of(obj, struct ttm_buffer_object, base);

	return ttm_bo_vmap(bo, map);
}

void
drm_gem_ttm_vunmap(struct drm_gem_object *obj, struct iosys_map *map)
{
	struct ttm_buffer_object *bo =
	    container_of(obj, struct ttm_buffer_object, base);

	ttm_bo_vunmap(bo, map);
}

/*	$NetBSD: drm_gem_vm.c,v 1.15 2022/07/06 01:12:45 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_gem_vm.c,v 1.15 2022/07/06 01:12:45 riastradh Exp $");

#include <sys/types.h>
#include <sys/file.h>
#include <sys/mman.h>

#include <uvm/uvm_extern.h>

#include <drm/drm_drv.h>
#include <drm/drm_gem.h>
#include <drm/drm_vma_manager.h>

void
drm_gem_pager_reference(struct uvm_object *uobj)
{
	struct drm_gem_object *const obj = container_of(uobj,
	    struct drm_gem_object, gemo_uvmobj);

	drm_gem_object_get(obj);
}

void
drm_gem_pager_detach(struct uvm_object *uobj)
{
	struct drm_gem_object *const obj = container_of(uobj,
	    struct drm_gem_object, gemo_uvmobj);

	drm_gem_object_put(obj);
}

int
drm_gem_mmap_object(struct drm_device *dev, off_t byte_offset, size_t nbytes,
    int prot, struct uvm_object **uobjp, voff_t *uoffsetp, struct file *file)
{
	struct drm_file *drm_file = file->f_data;
	const unsigned long startpage = (byte_offset >> PAGE_SHIFT);
	const unsigned long npages = (nbytes >> PAGE_SHIFT);
	struct drm_gem_object *obj = NULL;
	struct drm_vma_offset_node *node;

	KASSERT(drm_core_check_feature(dev, DRIVER_GEM));
	KASSERT(prot == (prot & (PROT_READ | PROT_WRITE)));
	KASSERT(0 <= byte_offset);
	KASSERT(byte_offset == (byte_offset & ~(PAGE_SIZE-1)));
	KASSERT(nbytes == (npages << PAGE_SHIFT));
	KASSERT(nbytes > 0);

	drm_vma_offset_lock_lookup(dev->vma_offset_manager);
	node = drm_vma_offset_exact_lookup_locked(dev->vma_offset_manager,
	    startpage, npages);
	if (node != NULL) {
		obj = container_of(node, struct drm_gem_object, vma_node);
		if (!kref_get_unless_zero(&obj->refcount))
			obj = NULL;
	}
	drm_vma_offset_unlock_lookup(dev->vma_offset_manager);

	if (node == NULL) {
		/* Fall back to vanilla device mappings.  */
		*uobjp = NULL;
		*uoffsetp = (voff_t)-1;
		return 0;
	}
	if (obj == NULL)
		return -EINVAL;

	if (!drm_vma_node_is_allowed(node, drm_file)) {
		drm_gem_object_put(obj);
		return -EACCES;
	}

	KASSERT(obj->dev == dev);
	if (obj->funcs == NULL || obj->funcs->vm_ops == NULL) {
		drm_gem_object_put(obj);
		return -EINVAL;
	}

	if (obj->gemo_uvmobj.pgops == NULL)
		obj->gemo_uvmobj.pgops = obj->funcs->vm_ops;
	else
		KASSERT(obj->gemo_uvmobj.pgops == obj->funcs->vm_ops);

	/* Success!  */
	*uobjp = &obj->gemo_uvmobj;
	*uoffsetp = 0;
	return 0;
}

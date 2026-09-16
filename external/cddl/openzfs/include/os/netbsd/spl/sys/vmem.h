/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_VMEM_H_
#define	_NETBSD_SPL_VMEM_H_

#include_next <sys/vmem.h>
#include <sys/kmem.h>

/* OpenZFS uses vmem for large virtually contiguous allocations. */
#define	vmem_alloc(size, flags)	kmem_alloc((size), (flags))
#define	vmem_zalloc(size, flags)	kmem_zalloc((size), (flags))
#define	vmem_free(buf, size)	kmem_free((buf), (size))

#endif

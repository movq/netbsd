/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_UIO_H_
#define	_NETBSD_SPL_UIO_H_

/* Keep the fallback's native types, but not its old zfs_ui* interfaces. */
#define	zfs_uiomove	netbsd_legacy_uiomove
#define	zfs_uiocopy	netbsd_legacy_uiocopy
#define	zfs_uioskip	netbsd_legacy_uioskip
#include_next <sys/uio.h>
#undef zfs_uiomove
#undef zfs_uiocopy
#undef zfs_uioskip
#undef uiomove
#undef uiocopy
#undef uioskip

#include <uvm/uvm_extern.h>

typedef enum uio_rw	zfs_uio_rw_t;
typedef enum uio_seg	zfs_uio_seg_t;

/* Extended flags are separate from the native uio; no pages can be pinned. */
#define	UIO_DIRECT	0x0001
typedef struct zfs_uio {
	struct uio	*uio;
	offset_t	uio_soffset;
	uint16_t	uio_extflg;
} zfs_uio_t;

#define	GET_UIO_STRUCT(u)	((u)->uio)
#define	zfs_uio_offset(u)	(GET_UIO_STRUCT(u)->uio_offset)
#define	zfs_uio_resid(u)		(GET_UIO_STRUCT(u)->uio_resid)
#define	zfs_uio_iovcnt(u)	(GET_UIO_STRUCT(u)->uio_iovcnt)
#define	zfs_uio_iovlen(u, i)	(GET_UIO_STRUCT(u)->uio_iov[(i)].iov_len)
#define	zfs_uio_iovbase(u, i)	(GET_UIO_STRUCT(u)->uio_iov[(i)].iov_base)
#define	zfs_uio_rw(u)		(GET_UIO_STRUCT(u)->uio_rw)
#define	zfs_uio_segflg(u) \
	(VMSPACE_IS_KERNEL_P(GET_UIO_STRUCT(u)->uio_vmspace) ? \
	UIO_SYSSPACE : UIO_USERSPACE)
#define	zfs_uio_soffset(u)	((u)->uio_soffset)

/* As in the old NetBSD port, native uiomove handles faults synchronously. */
#define	zfs_uio_fault_disable(u, set)	((void)0)
#define	zfs_uio_prefaultpages(size, u)	(0)

static inline void
zfs_uio_init(zfs_uio_t *uio, struct uio *native)
{
	memset(uio, 0, sizeof (*uio));
	uio->uio = native;
	if (native != NULL)
		uio->uio_soffset = native->uio_offset;
}

static inline void
zfs_uio_setoffset(zfs_uio_t *uio, offset_t off)
{
	zfs_uio_offset(uio) = off;
}

static inline void
zfs_uio_setsoffset(zfs_uio_t *uio, offset_t off)
{
	ASSERT3S(zfs_uio_offset(uio), ==, off);
	uio->uio_soffset = off;
}

static inline void
zfs_uio_advance(zfs_uio_t *uio, ssize_t size)
{
	zfs_uio_resid(uio) -= size;
	zfs_uio_offset(uio) += size;
}

int zfs_uio_fault_move(void *, size_t, zfs_uio_rw_t, zfs_uio_t *);

#endif

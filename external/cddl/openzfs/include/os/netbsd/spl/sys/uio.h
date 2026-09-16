/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_UIO_H_
#define	_NETBSD_SPL_UIO_H_

/*
 * Start with the existing NetBSD buffered-I/O interface. Direct I/O needs
 * additional OS support before it can be enabled.
 */
#include_next <sys/uio.h>

typedef uio_t		zfs_uio_t;
typedef enum uio_rw	zfs_uio_rw_t;
typedef enum uio_seg	zfs_uio_seg_t;

#define	GET_UIO_STRUCT(u)	(u)
#define	zfs_uio_offset(u)	((u)->uio_offset)
#define	zfs_uio_resid(u)		((u)->uio_resid)
#define	zfs_uio_iovcnt(u)	((u)->uio_iovcnt)
#define	zfs_uio_iovlen(u, i)	((u)->uio_iov[(i)].iov_len)
#define	zfs_uio_iovbase(u, i)	((u)->uio_iov[(i)].iov_base)
#define	zfs_uio_rw(u)		((u)->uio_rw)
#define	zfs_uio_segflg(u)	((u)->uio_segflg)

#endif

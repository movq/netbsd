/* SPDX-License-Identifier: BSD-2-Clause */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/uio_impl.h>

int
zfs_uiomove(void *buf, size_t size, zfs_uio_rw_t rw, zfs_uio_t *uio)
{
	ASSERT3U(zfs_uio_rw(uio), ==, rw);
	return (uiomove(buf, size, GET_UIO_STRUCT(uio)));
}

int
zfs_uio_fault_move(void *buf, size_t size, zfs_uio_rw_t rw, zfs_uio_t *uio)
{
	return (zfs_uiomove(buf, size, rw, uio));
}

/*
 * Copy through local iovecs so neither the native uio nor its iovecs advance.
 * Account for completed vectors even if a later user-memory access faults.
 */
int
zfs_uiocopy(void *buf, size_t size, zfs_uio_rw_t rw, zfs_uio_t *uio,
    size_t *copied)
{
	struct uio clone = *GET_UIO_STRUCT(uio);
	char *p = buf;

	ASSERT3U(zfs_uio_rw(uio), ==, rw);
	*copied = 0;
	for (int i = 0; i < zfs_uio_iovcnt(uio) && size != 0 &&
	    clone.uio_resid != 0; i++) {
		struct iovec iov = GET_UIO_STRUCT(uio)->uio_iov[i];
		size_t count = MIN(size, MIN(iov.iov_len, clone.uio_resid));
		size_t before = clone.uio_resid;
		int error;

		if (count == 0)
			continue;
		clone.uio_iov = &iov;
		clone.uio_iovcnt = 1;
		error = uiomove(p, count, &clone);
		count = before - clone.uio_resid;
		*copied += count;
		if (error != 0)
			return (error);
		p += count;
		size -= count;
	}
	return (0);
}

void
zfs_uioskip(zfs_uio_t *uio, size_t size)
{
	struct uio *native = GET_UIO_STRUCT(uio);

	if (size > native->uio_resid)
		return;
	while (size != 0) {
		struct iovec *iov = native->uio_iov;
		size_t count;

		ASSERT3S(native->uio_iovcnt, >, 0);
		if (iov->iov_len == 0) {
			native->uio_iov++;
			native->uio_iovcnt--;
			continue;
		}
		count = MIN(size, iov->iov_len);
		iov->iov_base = (char *)iov->iov_base + count;
		iov->iov_len -= count;
		native->uio_offset += count;
		native->uio_resid -= count;
		size -= count;
	}
}

boolean_t
zfs_uio_page_aligned(zfs_uio_t *uio)
{
	for (int i = 0; i < zfs_uio_iovcnt(uio); i++) {
		if (!IS_P2ALIGNED((uintptr_t)zfs_uio_iovbase(uio, i), PAGE_SIZE) ||
		    !IS_P2ALIGNED(zfs_uio_iovlen(uio, i), PAGE_SIZE))
			return (B_FALSE);
	}
	return (B_TRUE);
}

int
zfs_uio_get_dio_pages_alloc(zfs_uio_t *uio, zfs_uio_rw_t rw)
{
	return (ENOTSUP);
}

void
zfs_uio_free_dio_pages(zfs_uio_t *uio, zfs_uio_rw_t rw)
{
	panic("OpenZFS: direct I/O pages are unsupported on NetBSD");
}

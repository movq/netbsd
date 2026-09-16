/* SPDX-License-Identifier: BSD-2-Clause */

#include <sys/dmu_impl.h>
#include <sys/uio_impl.h>

/*
 * The vnode layer routes requests through the ARC.  If a caller incorrectly
 * marks a uio as direct, fail without transferring data or advancing the uio.
 */
int
dmu_read_uio_direct(dnode_t *dn, zfs_uio_t *uio, uint64_t size,
    dmu_flags_t flags)
{
	return (SET_ERROR(ENOTSUP));
}

int
dmu_write_uio_direct(dnode_t *dn, zfs_uio_t *uio, uint64_t size,
    dmu_flags_t flags, dmu_tx_t *tx)
{
	return (SET_ERROR(ENOTSUP));
}

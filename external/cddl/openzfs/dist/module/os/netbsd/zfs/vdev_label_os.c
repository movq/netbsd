/* SPDX-License-Identifier: CDDL-1.0 */
/*
 * Boot-reserve check adapted from OpenZFS 2.4.4's FreeBSD integration.
 *
 * NetBSD installboot does not put ZFS boot code in this area. Pools imported
 * from FreeBSD may contain BTX here, so retain the check before RAIDZ
 * expansion repurposes the reserved space.
 */
#include <sys/zfs_context.h>
#include <sys/spa_impl.h>
#include <sys/vdev_impl.h>
#include <sys/zio.h>
#include <sys/abd.h>

static void
vdev_boot_read_done(zio_t *zio)
{
	zio_t *parent = zio->io_private;

	mutex_enter(&parent->io_lock);
	parent->io_error = zio_worst_error(parent->io_error, zio->io_error);
	mutex_exit(&parent->io_lock);
}

int
vdev_check_boot_reserve(spa_t *spa, vdev_t *vd)
{
	size_t size = 1ULL << vd->vdev_top->vdev_ashift;
	abd_t *abd = abd_alloc_linear(size, B_FALSE);
	int flags = ZIO_FLAG_CANFAIL | ZIO_FLAG_TRYHARD;
	zio_t *zio = zio_root(spa, NULL, NULL, flags);
	int error;

	ASSERT(vd->vdev_ops->vdev_op_leaf);
	/* Child I/O adds VDEV_LABEL_START_SIZE to the supplied offset. */
	zio_nowait(zio_vdev_child_io(zio, NULL, vd,
	    VDEV_BOOT_OFFSET - VDEV_LABEL_START_SIZE, abd, size, ZIO_TYPE_READ,
	    ZIO_PRIORITY_ASYNC_READ, flags, vdev_boot_read_done, zio));
	error = zio_wait(zio);
	if (error == 0) {
		const unsigned char *buf = abd_to_buf(abd);

		if (buf[0] == 0xeb && buf[1] == 0x0e &&
		    buf[2] == 'B' && buf[3] == 'T' && buf[4] == 'X')
			error = EBUSY;
	}
	abd_free(abd);
	return (error);
}

/* SPDX-License-Identifier: BSD-2-Clause */

#include <sys/zfs_context.h>
#include <sys/module.h>
#include <sys/kstat.h>
#include <sys/taskq.h>
#include <sys/spa.h>
#include <sys/zfs_vfsops.h>
#include <sys/zfs_ioctl_impl.h>
#include <sys/zfs_ioctl_os.h>
#include <sys/zio.h>
#include <sys/zvol.h>

MODULE(MODULE_CLASS_VFS, zfs, "solaris");

extern struct vfsops zfs_vfsops;

static boolean_t
zfs_pools_busy(void)
{
	spa_t *spa;

	spa_namespace_enter(FTAG);
	for (spa = spa_next(NULL); spa != NULL; spa = spa_next(spa)) {
		if (spa_state(spa) != POOL_STATE_UNINITIALIZED)
			break;
	}
	spa_namespace_exit(FTAG);
	return (spa != NULL);
}

static int
zfs_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = spl_kstat_init();
		if (error != 0)
			return (error);
		taskq_init();
		error = zfs_kmod_init();
		if (error != 0)
			goto fini_spl;
		error = vfs_attach(&zfs_vfsops);
		if (error == 0)
			return (0);
		zfs_kmod_fini();
fini_spl:
		taskq_fini();
		spl_kstat_fini();
		return (error);
	case MODULE_CMD_FINI:
		if (zfs_pools_busy() || zfs_busy() || zvol_busy() || zfsdev_busy() ||
		    zio_injection_enabled)
			return (EBUSY);
		error = vfs_detach(&zfs_vfsops);
		if (error != 0)
			return (error);
		zfs_kmod_fini();
		taskq_fini();
		spl_kstat_fini();
		return (0);
	case MODULE_CMD_AUTOUNLOAD:
		/* Pool configuration can create devices without mounting a VFS. */
		return (EBUSY);
	default:
		return (ENOTTY);
	}
}

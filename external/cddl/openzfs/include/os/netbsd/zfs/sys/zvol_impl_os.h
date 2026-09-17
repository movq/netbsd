/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_ZVOL_IMPL_OS_H_
#define	_NETBSD_ZVOL_IMPL_OS_H_

#include <sys/zvol.h>
#include <sys/dataset_kstats.h>
#include <sys/zfs_rlock.h>
#include <sys/zil.h>
#include <sys/zvol_impl.h>
#include <sys/disk.h>

struct zvol_state_os {
	list_node_t	zos_link;
	zvol_state_t	*zos_zv;
	vmem_addr_t	zos_minor;
	struct disk	zos_disk;
	kmutex_t	zos_disk_lock;
	unsigned int	zos_openmask;	/* one final close per device type */
	unsigned int	zos_refs;	/* protected by native device-map lock */
};

/* Return with zv_suspend_lock (reader) and zv_state_lock held. */
zvol_state_t *zvol_os_hold(dev_t, boolean_t);
/* Drop the above locks, then the native device-map reference. */
void zvol_os_rele(zvol_state_t *);

#endif

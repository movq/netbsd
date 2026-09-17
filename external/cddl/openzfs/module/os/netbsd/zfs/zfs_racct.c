/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Charge filesystem I/O to the calling LWP and to the pool.
 */
#include <sys/zfs_context.h>
#include <sys/zfs_racct.h>
#include <sys/resourcevar.h>

void
zfs_racct_read(spa_t *spa, uint64_t size, uint64_t iops, dmu_flags_t flags)
{
	curlwp->l_ru.ru_inblock += iops;
	spa_iostats_read_add(spa, size, iops, flags);
}

void
zfs_racct_write(spa_t *spa, uint64_t size, uint64_t iops, dmu_flags_t flags)
{
	curlwp->l_ru.ru_oublock += iops;
	spa_iostats_write_add(spa, size, iops, flags);
}

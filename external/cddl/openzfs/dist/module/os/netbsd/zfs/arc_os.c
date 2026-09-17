// SPDX-License-Identifier: CDDL-1.0
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or https://opensource.org/licenses/CDDL-1.0.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, Joyent, Inc. All rights reserved.
 * Copyright (c) 2011, 2016 by Delphix. All rights reserved.
 * Copyright (c) 2014 by Saso Kiselkov. All rights reserved.
 * Copyright 2015 Nexenta Systems, Inc. All rights reserved.
 *
 * UVM accounting and pageout throttling from the NetBSD osnet ARC.
 */

#include <sys/zfs_context.h>
#include <sys/arc.h>
#include <sys/arc_impl.h>
#include <sys/dmu_tx.h>
#include <sys/spa_impl.h>
#include <uvm/uvm.h>

uint64_t
arc_all_memory(void)
{
	return ((uint64_t)physmem * PAGE_SIZE);
}

uint64_t
arc_free_memory(void)
{
	return ((uint64_t)uvm_availmem(false) * PAGE_SIZE);
}

int64_t
arc_available_memory(void)
{
	int64_t available, kva_free, kva_alloc;

	/* Begin reclaim before UVM drops below its free-page target. */
	available = ((int64_t)uvm_availmem(false) - uvmexp.freetarg) *
	    PAGE_SIZE;
	kva_free = vmem_size(kmem_arena, VMEM_FREE);
	kva_alloc = vmem_size(kmem_arena, VMEM_ALLOC);
#ifndef _LP64
	/* Retain osnet's larger KVA reserve on 32-bit kernels. */
	available = MIN(available, kva_free - (kva_free + kva_alloc) / 4);
#else
	/* Leave room for contiguous virtual mappings and fragmentation. */
	available = MIN(available, kva_free - kva_alloc / 16);
#endif
	return (available);
}

uint64_t
arc_default_max(uint64_t min, uint64_t allmem)
{
	uint64_t size = allmem >= (1ULL << 30) ?
	    allmem - (1ULL << 30) : min;

	return (MAX(allmem * 5 / 8, size));
}

int
arc_memory_throttle(spa_t *spa, uint64_t reserve, uint64_t txg)
{
	uint64_t available = arc_free_memory();

#ifndef _LP64
	available = MIN(available, vmem_size(kmem_arena, VMEM_FREE));
#endif
	if (available > arc_all_memory() * arc_lotsfree_percent / 100)
		return (0);

	/* OpenZFS tracks the old pageout allowance separately for each pool. */
	if (txg > spa->spa_lowmem_last_txg) {
		spa->spa_lowmem_last_txg = txg;
		spa->spa_lowmem_page_load = 0;
	}
	if (uvm_lwp_is_pagedaemon(curlwp)) {
		if (spa->spa_lowmem_page_load >
		    MAX((uint64_t)uvmexp.freemin * PAGE_SIZE, available) / 4) {
			DMU_TX_STAT_BUMP(dmu_tx_memory_reclaim);
			return (SET_ERROR(ERESTART));
		}
		atomic_add_64(&spa->spa_lowmem_page_load, reserve / 8);
		return (0);
	}
	if (spa->spa_lowmem_page_load > 0 && arc_reclaim_needed()) {
		ARCSTAT_BUMP(arcstat_memory_throttle_count);
		DMU_TX_STAT_BUMP(dmu_tx_memory_reclaim);
		return (SET_ERROR(EAGAIN));
	}
	spa->spa_lowmem_page_load = 0;
	return (0);
}

/*
 * The old NetBSD port polled UVM from the ARC reclaim thread. The common
 * OpenZFS reap thread likewise polls arc_available_memory(); there is no
 * FreeBSD vm_lowmem event handler to register on NetBSD.
 */
void
arc_lowmem_init(void)
{
}

void
arc_lowmem_fini(void)
{
}

/* NetBSD has no memory hotplug notification used by this integration. */
void
arc_register_hotplug(void)
{
}

void
arc_unregister_hotplug(void)
{
}

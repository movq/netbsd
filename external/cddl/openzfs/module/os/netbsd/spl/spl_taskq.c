/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Derived from the NetBSD Solaris taskq implementation contributed by
 * Juergen Hannken-Illjes.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/callout.h>
#include <sys/condvar.h>
#include <sys/debug.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/taskq.h>
#include <sys/threadpool.h>
#include <sys/time.h>
#include <sys/tsd.h>

struct taskq_executor {
	struct threadpool_job te_job;	/* Must be first. */
	taskq_t		*te_queue;
	kthread_t	*te_thread;
	taskqid_t	te_id;		/* Executing callback, independent of entry. */
	boolean_t	te_running;	/* Job scheduled or running. */
};

TAILQ_HEAD(taskq_list, openzfs_taskq_ent);
struct taskq {
	kmutex_t	tq_lock;
	kcondvar_t	tq_cv;		/* Completion and executor startup. */
	kcondvar_t	tq_work_cv;
	struct threadpool *tq_pool;
	struct taskq_executor *tq_executor;
	pri_t		tq_pri;
	int		tq_nthreads;
	int		tq_running;
	uint64_t	tq_active;	/* Delayed, queued, and executing callbacks. */
	taskqid_t	tq_lastid;
	boolean_t	tq_pinned;	/* Synced queues retain worker identities. */
	boolean_t	tq_destroyed;
	struct taskq_list tq_ready;
	struct taskq_list tq_delayed;	/* Ordered by expiry. */
	avl_tree_t	tq_pending;	/* Queued/delayed entries, ordered by ID. */
	callout_t	tq_timer;	/* References the queue, never an entry. */
};

taskq_t *system_taskq;
taskq_t *system_delay_taskq;
static uint_t taskq_tsd;
static uint64_t taskq_nextid;

static void taskq_executor(struct threadpool_job *);

static int
taskq_compare(const void *a, const void *b)
{
	const taskq_ent_t *ea = a, *eb = b;

	return ((ea->tqent_id > eb->tqent_id) -
	    (ea->tqent_id < eb->tqent_id));
}

/* Native clock_t is unsigned; deadlines must be within INT_MAX ticks. */
static int
taskq_ticks(clock_t deadline)
{
	CTASSERT(sizeof (clock_t) == sizeof (int));
	return ((int)(deadline - ddi_get_lbolt()));
}

static taskq_ent_t *
taskq_find(taskq_t *tq, taskqid_t id)
{
	taskq_ent_t key = { .tqent_id = id };

	return (avl_find(&tq->tq_pending, &key, NULL));
}

static boolean_t
taskq_executing(taskq_t *tq, taskqid_t id)
{
	if (id == TASKQID_INVALID)
		return (B_FALSE);
	for (int i = 0; i < tq->tq_nthreads; i++)
		if (tq->tq_executor[i].te_id == id)
			return (B_TRUE);
	return (B_FALSE);
}

static void
taskq_kick(taskq_t *tq)
{
	ASSERT(MUTEX_HELD(&tq->tq_lock));
	if (tq->tq_pinned) {
		cv_broadcast(&tq->tq_work_cv);
		return;
	}
	for (int i = 0; i < tq->tq_nthreads; i++) {
		struct taskq_executor *te = &tq->tq_executor[i];
		if (!te->te_running) {
			te->te_running = B_TRUE;
			tq->tq_running++;
			threadpool_schedule_job(tq->tq_pool, &te->te_job);
			break;
		}
	}
}

static void
taskq_timer_update(taskq_t *tq)
{
	taskq_ent_t *ent = TAILQ_FIRST(&tq->tq_delayed);

	if (ent == NULL)
		(void) callout_stop(&tq->tq_timer);
	else
		callout_schedule(&tq->tq_timer,
		    MAX(1, taskq_ticks(ent->tqent_expire)));
}

/* Only make work runnable in softclock context; callbacks run in workers. */
static void
taskq_expire(void *arg)
{
	taskq_t *tq = arg;
	taskq_ent_t *ent;

	mutex_enter(&tq->tq_lock);
	while ((ent = TAILQ_FIRST(&tq->tq_delayed)) != NULL &&
	    taskq_ticks(ent->tqent_expire) <= 0) {
		TAILQ_REMOVE(&tq->tq_delayed, ent, tqent_list);
		ent->tqent_delayed = B_FALSE;
		TAILQ_INSERT_TAIL(&tq->tq_ready, ent, tqent_list);
		taskq_kick(tq);
	}
	taskq_timer_update(tq);
	mutex_exit(&tq->tq_lock);
}

static void
taskq_executor(struct threadpool_job *job)
{
	struct taskq_executor *te = (struct taskq_executor *)job;
	taskq_t *tq = te->te_queue;

	tsd_set(taskq_tsd, tq);
	mutex_enter(&tq->tq_lock);
	te->te_thread = curthread;
	cv_broadcast(&tq->tq_cv);
	for (;;) {
		taskq_ent_t *ent = TAILQ_FIRST(&tq->tq_ready);
		if (ent == NULL) {
			if (!tq->tq_pinned || tq->tq_destroyed)
				break;
			cv_wait(&tq->tq_work_cv, &tq->tq_lock);
			continue;
		}

		task_func_t *func = ent->tqent_func;
		void *arg = ent->tqent_arg;
		boolean_t dynamic = ent->tqent_dynamic;
		te->te_id = ent->tqent_id;
		TAILQ_REMOVE(&tq->tq_ready, ent, tqent_list);
		avl_remove(&tq->tq_pending, ent);
		atomic_store_release(&ent->tqent_queued, 0);
		/*
		 * In particular, dbuf and ZIO callbacks can free or redispatch
		 * their embedded entry. Keep all running state in the executor.
		 */
		if (dynamic)
			kmem_free(ent, sizeof (*ent));
		mutex_exit(&tq->tq_lock);
		func(arg);
		mutex_enter(&tq->tq_lock);
		te->te_id = TASKQID_INVALID;
		tq->tq_active--;
		cv_broadcast(&tq->tq_cv);
	}
	te->te_thread = NULL;
	te->te_running = B_FALSE;
	tq->tq_running--;
	/* The queue and its TSD key may be destroyed after job_done. */
	tsd_set(taskq_tsd, NULL);
	cv_broadcast(&tq->tq_cv);
	threadpool_job_done(job);
	mutex_exit(&tq->tq_lock);
}

static taskqid_t
taskq_enqueue(taskq_t *tq, taskq_ent_t *ent, uint_t flags, clock_t expire,
    boolean_t delayed)
{
	ASSERT(MUTEX_HELD(&tq->tq_lock));
	ASSERT(!tq->tq_destroyed);
	ASSERT(taskq_empty_ent(ent));
	ent->tqent_id = atomic_inc_64_nv(&taskq_nextid);
	/* Never reuse an ID, even on machines with 32-bit pointers. */
	VERIFY3U(ent->tqent_id, !=, TASKQID_INVALID);
	tq->tq_lastid = ent->tqent_id;
	tq->tq_active++;
	avl_add(&tq->tq_pending, ent);
	ent->tqent_delayed = delayed && taskq_ticks(expire) > 0;
	atomic_store_release(&ent->tqent_queued, 1);
	if (ent->tqent_delayed) {
		taskq_ent_t *pos;
		clock_t now = ddi_get_lbolt();
		ent->tqent_expire = expire;
		TAILQ_FOREACH(pos, &tq->tq_delayed, tqent_list)
			if ((int)(expire - now) <
			    (int)(pos->tqent_expire - now))
				break;
		if (pos != NULL)
			TAILQ_INSERT_BEFORE(pos, ent, tqent_list);
		else
			TAILQ_INSERT_TAIL(&tq->tq_delayed, ent, tqent_list);
		taskq_timer_update(tq);
	} else {
		if (flags & TQ_FRONT)
			TAILQ_INSERT_HEAD(&tq->tq_ready, ent, tqent_list);
		else
			TAILQ_INSERT_TAIL(&tq->tq_ready, ent, tqent_list);
		taskq_kick(tq);
	}
	return (ent->tqent_id);
}

static taskqid_t
taskq_dispatch_impl(taskq_t *tq, task_func_t func, void *arg, uint_t flags,
    clock_t expire, boolean_t delayed)
{
	taskq_ent_t *ent;
	taskqid_t id;

	VERIFY0(flags & ~(TQ_NOSLEEP | TQ_NOQUEUE | TQ_NOALLOC | TQ_FRONT));
	/* Allocation hints are not implemented; NOALLOC must never allocate. */
	if (flags & TQ_NOALLOC)
		return (TASKQID_INVALID);
	ent = kmem_zalloc(sizeof (*ent),
	    flags & (TQ_NOSLEEP | TQ_NOQUEUE) ? KM_NOSLEEP : KM_SLEEP);
	if (ent == NULL)
		return (TASKQID_INVALID);
	ent->tqent_func = func;
	ent->tqent_arg = arg;
	ent->tqent_dynamic = B_TRUE;
	mutex_enter(&tq->tq_lock);
	if ((flags & TQ_NOQUEUE) && tq->tq_active >= tq->tq_nthreads) {
		mutex_exit(&tq->tq_lock);
		kmem_free(ent, sizeof (*ent));
		return (TASKQID_INVALID);
	}
	id = taskq_enqueue(tq, ent, flags, expire, delayed);
	mutex_exit(&tq->tq_lock);
	return (id);
}

taskqid_t
taskq_dispatch(taskq_t *tq, task_func_t func, void *arg, uint_t flags)
{
	return (taskq_dispatch_impl(tq, func, arg, flags, 0, B_FALSE));
}

taskqid_t
taskq_dispatch_delay(taskq_t *tq, task_func_t func, void *arg, uint_t flags,
    clock_t expire)
{
	return (taskq_dispatch_impl(tq, func, arg, flags, expire, B_TRUE));
}

void
taskq_dispatch_ent(taskq_t *tq, task_func_t func, void *arg, uint_t flags,
    taskq_ent_t *ent)
{
	VERIFY0(flags & ~(TQ_NOSLEEP | TQ_NOQUEUE | TQ_NOALLOC | TQ_FRONT));
	mutex_enter(&tq->tq_lock);
	ASSERT(taskq_empty_ent(ent));
	ent->tqent_func = func;
	ent->tqent_arg = arg;
	ent->tqent_dynamic = B_FALSE;
	(void) taskq_enqueue(tq, ent, flags, 0, B_FALSE);
	mutex_exit(&tq->tq_lock);
}

void
taskq_init_ent(taskq_ent_t *ent)
{
	memset(ent, 0, sizeof (*ent));
}

int
taskq_empty_ent(taskq_ent_t *ent)
{
	return (atomic_load_acquire(&ent->tqent_queued) == 0);
}

int
taskq_cancel_id(taskq_t *tq, taskqid_t id, boolean_t wait)
{
	taskq_ent_t *ent;

	mutex_enter(&tq->tq_lock);
	if ((ent = taskq_find(tq, id)) != NULL) {
		if (ent->tqent_delayed) {
			TAILQ_REMOVE(&tq->tq_delayed, ent, tqent_list);
			taskq_timer_update(tq);
		} else {
			TAILQ_REMOVE(&tq->tq_ready, ent, tqent_list);
		}
		avl_remove(&tq->tq_pending, ent);
		boolean_t dynamic = ent->tqent_dynamic;
		atomic_store_release(&ent->tqent_queued, 0);
		if (dynamic)
			kmem_free(ent, sizeof (*ent));
		tq->tq_active--;
		cv_broadcast(&tq->tq_cv);
		mutex_exit(&tq->tq_lock);
		return (0);	/* Callback was prevented from running. */
	}
	if (taskq_executing(tq, id)) {
		if (!wait) {
			mutex_exit(&tq->tq_lock);
			return (EBUSY);
		}
		ASSERT3P(taskq_of_curthread(), !=, tq);
		do {
			cv_wait(&tq->tq_cv, &tq->tq_lock);
		} while (taskq_executing(tq, id));
	}
	mutex_exit(&tq->tq_lock);
	/* Already executed, including callbacks we just waited for. */
	return (ENOENT);
}

void
taskq_wait_id(taskq_t *tq, taskqid_t id)
{
	ASSERT3P(taskq_of_curthread(), !=, tq);
	mutex_enter(&tq->tq_lock);
	while (taskq_find(tq, id) != NULL || taskq_executing(tq, id))
		cv_wait(&tq->tq_cv, &tq->tq_lock);
	mutex_exit(&tq->tq_lock);
}

static boolean_t
taskq_outstanding(taskq_t *tq, taskqid_t id)
{
	taskq_ent_t *ent = avl_first(&tq->tq_pending);

	if (ent != NULL && ent->tqent_id <= id)
		return (B_TRUE);
	for (int i = 0; i < tq->tq_nthreads; i++) {
		taskqid_t running = tq->tq_executor[i].te_id;
		if (running != TASKQID_INVALID && running <= id)
			return (B_TRUE);
	}
	return (B_FALSE);
}

void
taskq_wait_outstanding(taskq_t *tq, taskqid_t id)
{
	ASSERT3P(taskq_of_curthread(), !=, tq);
	mutex_enter(&tq->tq_lock);
	if (id == TASKQID_INVALID)
		id = tq->tq_lastid;
	while (taskq_outstanding(tq, id))
		cv_wait(&tq->tq_cv, &tq->tq_lock);
	mutex_exit(&tq->tq_lock);
}

void
taskq_wait(taskq_t *tq)
{
	ASSERT3P(taskq_of_curthread(), !=, tq);
	mutex_enter(&tq->tq_lock);
	while (tq->tq_active != 0)
		cv_wait(&tq->tq_cv, &tq->tq_lock);
	mutex_exit(&tq->tq_lock);
}

taskq_t *
taskq_of_curthread(void)
{
	return (tsd_get(taskq_tsd));
}

int
taskq_member(taskq_t *tq, kthread_t *thread)
{
	int member = 0;

	mutex_enter(&tq->tq_lock);
	for (int i = 0; i < tq->tq_nthreads; i++)
		if (thread != NULL && tq->tq_executor[i].te_thread == thread)
			member = 1;
	mutex_exit(&tq->tq_lock);
	return (member);
}

taskq_t *
taskq_create(const char *name, int nthreads, pri_t pri, int minalloc,
    int maxalloc, uint_t flags)
{
	struct threadpool *pool;
	taskq_t *tq;

	/* As in the old port, minalloc/maxalloc are allocation hints. */
	VERIFY0(flags & ~(TASKQ_PREPOPULATE | TASKQ_CPR_SAFE | TASKQ_DYNAMIC |
	    TASKQ_THREADS_CPU_PCT | TASKQ_DC_BATCH));
	if (flags & TASKQ_THREADS_CPU_PCT)
		nthreads = MAX(((uint64_t)ncpu * nthreads) / 100, 1);
	VERIFY3S(nthreads, >, 0);
	if (threadpool_get(&pool, pri) != 0)
		return (NULL);
	tq = kmem_zalloc(sizeof (*tq), KM_SLEEP);
	tq->tq_nthreads = nthreads;
	tq->tq_pri = pri;
	tq->tq_pool = pool;
	mutex_init(&tq->tq_lock, NULL, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&tq->tq_cv, NULL, CV_DEFAULT, NULL);
	cv_init(&tq->tq_work_cv, NULL, CV_DEFAULT, NULL);
	TAILQ_INIT(&tq->tq_ready);
	TAILQ_INIT(&tq->tq_delayed);
	avl_create(&tq->tq_pending, taskq_compare, sizeof (taskq_ent_t),
	    offsetof(taskq_ent_t, tqent_node));
	callout_init(&tq->tq_timer, CALLOUT_MPSAFE);
	callout_setfunc(&tq->tq_timer, taskq_expire, tq);
	tq->tq_executor = kmem_zalloc(sizeof (*tq->tq_executor) * nthreads,
	    KM_SLEEP);
	for (int i = 0; i < nthreads; i++) {
		struct taskq_executor *te = &tq->tq_executor[i];
		te->te_queue = tq;
		threadpool_job_init(&te->te_job, taskq_executor,
		    &tq->tq_lock, "%s/%d", name, i);
	}
	return (tq);
}

taskq_t *
taskq_create_proc(const char *name, int nthreads, pri_t pri, int minalloc,
    int maxalloc, struct proc *proc, uint_t flags)
{
	/* Native pool workers belong to the kernel process. */
	return (taskq_create(name, nthreads, pri, minalloc, maxalloc, flags));
}

taskq_t *
taskq_create_synced(const char *name, int nthreads, pri_t pri, int minalloc,
    int maxalloc, uint_t flags, kthread_t ***threads)
{
	taskq_t *tq;
	kthread_t **kt;

	flags &= ~(TASKQ_DYNAMIC | TASKQ_THREADS_CPU_PCT | TASKQ_DC_BATCH);
	*threads = NULL;
	tq = taskq_create(name, nthreads, pri, minalloc, maxalloc, flags);
	if (tq == NULL)
		return (NULL);
	kt = kmem_alloc(sizeof (*kt) * nthreads, KM_SLEEP);
	mutex_enter(&tq->tq_lock);
	/*
	 * spa_sync_tq keeps these identities for its allocator assignments.
	 * Retain the pool jobs until destruction, even while the queue is idle.
	 */
	tq->tq_pinned = B_TRUE;
	for (int i = 0; i < nthreads; i++) {
		struct taskq_executor *te = &tq->tq_executor[i];
		te->te_running = B_TRUE;
		tq->tq_running++;
		threadpool_schedule_job(tq->tq_pool, &te->te_job);
	}
	for (int i = 0; i < nthreads; i++) {
		while (tq->tq_executor[i].te_thread == NULL)
			cv_wait(&tq->tq_cv, &tq->tq_lock);
		kt[i] = tq->tq_executor[i].te_thread;
	}
	mutex_exit(&tq->tq_lock);
	*threads = kt;
	return (tq);
}

void
taskq_destroy(taskq_t *tq)
{
	ASSERT3P(taskq_of_curthread(), !=, tq);
	/* Producers must be stopped (and recurring timers cancelled) first. */
	mutex_enter(&tq->tq_lock);
	while (tq->tq_active != 0)
		cv_wait(&tq->tq_cv, &tq->tq_lock);
	tq->tq_destroyed = B_TRUE;
	cv_broadcast(&tq->tq_work_cv);
	while (tq->tq_running != 0)
		cv_wait(&tq->tq_cv, &tq->tq_lock);
	mutex_exit(&tq->tq_lock);
	/* A stopped callout may still be returning from taskq_expire. */
	(void) callout_halt(&tq->tq_timer, NULL);
	callout_destroy(&tq->tq_timer);
	for (int i = 0; i < tq->tq_nthreads; i++)
		threadpool_job_destroy(&tq->tq_executor[i].te_job);
	threadpool_put(tq->tq_pool, tq->tq_pri);
	avl_destroy(&tq->tq_pending);
	cv_destroy(&tq->tq_work_cv);
	cv_destroy(&tq->tq_cv);
	mutex_destroy(&tq->tq_lock);
	kmem_free(tq->tq_executor, sizeof (*tq->tq_executor) * tq->tq_nthreads);
	kmem_free(tq, sizeof (*tq));
}

void
taskq_init(void)
{
	tsd_create(&taskq_tsd, NULL);
	system_taskq = taskq_create("zfs_system", ncpu * 4, minclsyspri,
	    4, 512, TASKQ_DYNAMIC | TASKQ_PREPOPULATE);
	VERIFY3P(system_taskq, !=, NULL);
	system_delay_taskq = taskq_create("zfs_delay", ncpu, minclsyspri,
	    4, 512, TASKQ_DYNAMIC | TASKQ_PREPOPULATE);
	VERIFY3P(system_delay_taskq, !=, NULL);
}

void
taskq_fini(void)
{
	taskq_destroy(system_delay_taskq);
	taskq_destroy(system_taskq);
	system_delay_taskq = system_taskq = NULL;
	tsd_destroy(&taskq_tsd);
}

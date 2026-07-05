/*	$NetBSD: linux_kthread.c,v 1.9 2021/12/19 12:43:05 riastradh Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_kthread.c,v 1.9 2021/12/19 12:43:05 riastradh Exp $");

#include <sys/types.h>

#include <sys/condvar.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/lwp.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/specificdata.h>
#include <sys/stdarg.h>

#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>

#include <drm/drm_wait_netbsd.h>

struct task_struct {
	kmutex_t	kt_lock;
	kcondvar_t	kt_cv;
	bool		kt_shouldstop:1;
	bool		kt_shouldpark:1;
	bool		kt_parked:1;
	bool		kt_exited:1;
	int		kt_ret;

	int		(*kt_func)(void *);
	void		*kt_cookie;
	spinlock_t	*kt_interlock;
	drm_waitqueue_t	*kt_wq;
	struct lwp	*kt_lwp;
};

static specificdata_key_t linux_kthread_key __read_mostly = -1;

int
linux_kthread_init(void)
{
	int error;

	error = lwp_specific_key_create(&linux_kthread_key, NULL);
	if (error)
		goto out;

	/* Success!  */
	error = 0;

out:	if (error)
		linux_kthread_fini();
	return error;
}

void
linux_kthread_fini(void)
{

	if (linux_kthread_key != -1) {
		lwp_specific_key_delete(linux_kthread_key);
		linux_kthread_key = -1;
	}
}

#define	linux_kthread()	_linux_kthread(__func__)
static struct task_struct *
_linux_kthread(const char *caller)
{
	struct task_struct *T;

	T = lwp_getspecific(linux_kthread_key);
	KASSERTMSG(T != NULL, "%s must be called from Linux kthread", caller);

	return T;
}

static void
linux_kthread_start(void *cookie)
{
	struct task_struct *T = cookie;
	int ret;

	lwp_setspecific(linux_kthread_key, T);

	ret = (*T->kt_func)(T->kt_cookie);

	/*
	 * Mark the thread exited, set the return value, and wake any
	 * waiting kthread_stop.
	 */
	mutex_enter(&T->kt_lock);
	T->kt_exited = true;
	T->kt_ret = ret;
	cv_broadcast(&T->kt_cv);
	mutex_exit(&T->kt_lock);

	/* Exit the (NetBSD) kthread.  */
	kthread_exit(0);
}

static struct task_struct *
kthread_alloc(int (*func)(void *), void *cookie, spinlock_t *interlock,
    drm_waitqueue_t *wq)
{
	struct task_struct *T;

	T = kmem_zalloc(sizeof(*T), KM_SLEEP);

	mutex_init(&T->kt_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&T->kt_cv, "lnxkthrd");

	T->kt_shouldstop = false;
	T->kt_shouldpark = false;
	T->kt_parked = false;
	T->kt_exited = false;
	T->kt_ret = 0;

	T->kt_func = func;
	T->kt_cookie = cookie;
	T->kt_interlock = interlock;
	T->kt_wq = wq;

	return T;
}

static void
kthread_free(struct task_struct *T)
{

	KASSERT(T->kt_exited);

	cv_destroy(&T->kt_cv);
	mutex_destroy(&T->kt_lock);
	kmem_free(T, sizeof(*T));
}

struct task_struct *
kthread_run(int (*func)(void *), void *cookie, const char *name,
    spinlock_t *interlock, drm_waitqueue_t *wq)
{
	struct task_struct *T;
	int error;

	T = kthread_alloc(func, cookie, interlock, wq);
	error = kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL,
	    linux_kthread_start, T, &T->kt_lwp, "%s", name);
	if (error) {
		kthread_free(T);
		return ERR_PTR(-error); /* XXX errno NetBSD->Linux */
	}

	return T;
}

int
kthread_stop(struct task_struct *T)
{
	int ret;

	/* Lock order: interlock, then kthread lock.  */
	spin_lock(T->kt_interlock);
	mutex_enter(&T->kt_lock);

	/*
	 * Notify the thread that it's stopping, and wake it if it's
	 * parked or sleeping on its own waitqueue.
	 */
	T->kt_shouldpark = false;
	T->kt_shouldstop = true;
	cv_broadcast(&T->kt_cv);
	DRM_SPIN_WAKEUP_ALL(T->kt_wq, T->kt_interlock);

	/* Release the interlock while we wait for thread to finish.  */
	spin_unlock(T->kt_interlock);

	/* Wait for the thread to finish.  */
	while (!T->kt_exited)
		cv_wait(&T->kt_cv, &T->kt_lock);

	/* Grab the return code and release the lock -- we're done.  */
	ret = T->kt_ret;
	mutex_exit(&T->kt_lock);

	/* Free the (Linux) kthread.  */
	kthread_free(T);

	/* Return what the thread returned.  */
	return ret;
}

int
kthread_should_stop(void)
{
	struct task_struct *T = linux_kthread();
	bool shouldstop;

	mutex_enter(&T->kt_lock);
	shouldstop = T->kt_shouldstop;
	mutex_exit(&T->kt_lock);

	return shouldstop;
}

void
kthread_park(struct task_struct *T)
{

	/* Lock order: interlock, then kthread lock.  */
	spin_lock(T->kt_interlock);
	mutex_enter(&T->kt_lock);

	/* Caller must not ask to park if they've already asked to stop.  */
	KASSERT(!T->kt_shouldstop);

	/* Ask the thread to park.  */
	T->kt_shouldpark = true;

	/*
	 * Ensure the thread is not sleeping on its condvar.  After
	 * this point, we are done with the interlock, which we must
	 * not hold while we wait on the kthread condvar.
	 */
	DRM_SPIN_WAKEUP_ALL(T->kt_wq, T->kt_interlock);
	spin_unlock(T->kt_interlock);

	/*
	 * Wait until the thread has issued kthread_parkme, unless we
	 * are already the thread, which Linux allows and interprets to
	 * mean don't wait.
	 */
	if (T->kt_lwp != curlwp) {
		while (!T->kt_parked)
			cv_wait(&T->kt_cv, &T->kt_lock);
	}

	/* Release the kthread lock too.  */
	mutex_exit(&T->kt_lock);
}

void
kthread_unpark(struct task_struct *T)
{

	mutex_enter(&T->kt_lock);
	T->kt_shouldpark = false;
	cv_broadcast(&T->kt_cv);
	mutex_exit(&T->kt_lock);
}

int
__kthread_should_park(struct task_struct *T)
{
	bool shouldpark;

	mutex_enter(&T->kt_lock);
	shouldpark = T->kt_shouldpark;
	mutex_exit(&T->kt_lock);

	return shouldpark;
}

int
kthread_should_park(void)
{
	struct task_struct *T = linux_kthread();

	return __kthread_should_park(T);
}

void
kthread_parkme(void)
{
	struct task_struct *T = linux_kthread();

	assert_spin_locked(T->kt_interlock);

	spin_unlock(T->kt_interlock);
	mutex_enter(&T->kt_lock);
	while (T->kt_shouldpark) {
		T->kt_parked = true;
		cv_broadcast(&T->kt_cv);
		cv_wait(&T->kt_cv, &T->kt_lock);
		T->kt_parked = false;
	}
	mutex_exit(&T->kt_lock);
	spin_lock(T->kt_interlock);
}

/*
 * Kthread worker API
 *
 *	A kthread_worker is a dedicated kernel thread that processes
 *	kthread_work items from a queue.  This is similar to a
 *	single-threaded workqueue but uses the kthread_work type with
 *	a different callback signature (void (*)(struct kthread_work *)).
 */

TAILQ_HEAD(kw_work_head, kthread_work);

struct kthread_worker {
	kmutex_t		kw_lock;
	kcondvar_t		kw_cv;
	struct kw_work_head	kw_work_list;
	struct kthread_work	*kw_current_work;
	bool			kw_dying;
	struct lwp		*kw_lwp;
	struct task_struct	*kw_task;	/* for sched_set_fifo */
};

struct kthread_flush_work {
	struct kthread_work	kfw_work;
	kmutex_t		kfw_lock;
	kcondvar_t		kfw_cv;
	bool			kfw_done;
};

static void __dead	kthread_worker_thread(void *);

static bool
kthread_work_pending(const struct kthread_work *work)
{

	return work->entry.tqe_prev != NULL;
}

static void
kthread_work_entry_init(struct kthread_work *work)
{

	work->entry.tqe_next = NULL;
	work->entry.tqe_prev = NULL;
}

static void
kthread_remove_work(struct kthread_worker *worker,
    struct kthread_work *work)
{

	KASSERT(kthread_work_pending(work));
	TAILQ_REMOVE(&worker->kw_work_list, work, entry);
	kthread_work_entry_init(work);
}

static void
kthread_insert_work_tail(struct kthread_worker *worker,
    struct kthread_work *work)
{

	KASSERT(!kthread_work_pending(work));
	KASSERT(work->worker == NULL || work->worker == worker);

	TAILQ_INSERT_TAIL(&worker->kw_work_list, work, entry);
	work->worker = worker;
	cv_signal(&worker->kw_cv);
}

static void
kthread_insert_work_head(struct kthread_worker *worker,
    struct kthread_work *work)
{

	KASSERT(!kthread_work_pending(work));
	KASSERT(work->worker == NULL || work->worker == worker);

	TAILQ_INSERT_HEAD(&worker->kw_work_list, work, entry);
	work->worker = worker;
	cv_signal(&worker->kw_cv);
}

static void
kthread_insert_work_after(struct kthread_worker *worker,
    struct kthread_work *after, struct kthread_work *work)
{

	KASSERT(kthread_work_pending(after));
	KASSERT(!kthread_work_pending(work));
	KASSERT(work->worker == NULL || work->worker == worker);

	TAILQ_INSERT_AFTER(&worker->kw_work_list, after, work, entry);
	work->worker = worker;
	cv_signal(&worker->kw_cv);
}

static void
kthread_flush_work_cb(struct kthread_work *work)
{
	struct kthread_flush_work *const fwork = (void *)work;

	mutex_enter(&fwork->kfw_lock);
	fwork->kfw_done = true;
	cv_broadcast(&fwork->kfw_cv);
	mutex_exit(&fwork->kfw_lock);
}

static void
kthread_flush_work_init(struct kthread_flush_work *fwork)
{

	kthread_init_work(&fwork->kfw_work, &kthread_flush_work_cb);
	mutex_init(&fwork->kfw_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&fwork->kfw_cv, "kthrdfls");
	fwork->kfw_done = false;
}

static void
kthread_flush_work_fini(struct kthread_flush_work *fwork)
{

	cv_destroy(&fwork->kfw_cv);
	mutex_destroy(&fwork->kfw_lock);
}

static void
kthread_flush_work_wait(struct kthread_flush_work *fwork)
{

	mutex_enter(&fwork->kfw_lock);
	while (!fwork->kfw_done)
		cv_wait(&fwork->kfw_cv, &fwork->kfw_lock);
	mutex_exit(&fwork->kfw_lock);

	kthread_flush_work_fini(fwork);
}

/*
 * kthread_worker_thread(cookie)
 *
 *	Main function for a kthread worker's thread.  Waits for work
 *	items to be queued, dequeues and executes them one at a time,
 *	and repeats until the worker is marked dying.
 */
static void __dead
kthread_worker_thread(void *cookie)
{
	struct kthread_worker *const worker = cookie;
	struct kthread_work *work;
	void (*func)(struct kthread_work *);

	mutex_enter(&worker->kw_lock);
	for (;;) {
		if (worker->kw_dying)
			break;

		if (TAILQ_EMPTY(&worker->kw_work_list)) {
			cv_wait(&worker->kw_cv, &worker->kw_lock);
			continue;
		}

		work = TAILQ_FIRST(&worker->kw_work_list);
		kthread_remove_work(worker, work);
		worker->kw_current_work = work;
		func = work->func;

		mutex_exit(&worker->kw_lock);
		(*func)(work);
		mutex_enter(&worker->kw_lock);

		/*
		 * The callback may have freed work.  Only clear the worker's
		 * copy of its address after invoking it.
		 */
		worker->kw_current_work = NULL;
	}
	mutex_exit(&worker->kw_lock);

	kthread_exit(0);
}

/*
 * kthread_run_worker(flags, namefmt, ...)
 *
 *	Create a kthread worker and start its thread.  The namefmt and
 *	subsequent arguments are used to format the thread name.
 *	Return a pointer to the worker on success, or ERR_PTR(errno)
 *	on failure.
 */
struct kthread_worker *
kthread_run_worker(unsigned int flags, const char *namefmt, ...)
{
	struct kthread_worker *worker;
	va_list ap;
	char name[64];
	int error;

	(void)flags;		/* ignored for now */

	worker = kmem_zalloc(sizeof(*worker), KM_SLEEP);
	mutex_init(&worker->kw_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&worker->kw_cv, "kthrdwrk");
	TAILQ_INIT(&worker->kw_work_list);
	worker->kw_current_work = NULL;
	worker->kw_dying = false;

	va_start(ap, namefmt);
	vsnprintf(name, sizeof(name), namefmt, ap);
	va_end(ap);

	error = kthread_create(PRI_NONE, KTHREAD_MPSAFE | KTHREAD_MUSTJOIN, NULL,
	    &kthread_worker_thread, worker, &worker->kw_lwp, "%s", name);
	if (error) {
		cv_destroy(&worker->kw_cv);
		mutex_destroy(&worker->kw_lock);
		kmem_free(worker, sizeof(*worker));
		return ERR_PTR(-error);
	}

	/* Dummy task_struct pointer for sched_set_fifo compatibility. */
	worker->kw_task = (struct task_struct *)worker;

	return worker;
}

/*
 * kthread_destroy_worker(worker)
 *
 *	Destroy a kthread worker.  Flush all pending work, stop the
 *	thread, and free resources.  The worker must not be used after
 *	this call.
 */
void
kthread_destroy_worker(struct kthread_worker *worker)
{
	struct kthread_work *work;

	/*
	 * Put a boundary after work that is currently pending.  Stop after
	 * that boundary rather than trying to drain work that keeps rearming.
	 */
	kthread_flush_worker(worker);

	mutex_enter(&worker->kw_lock);
	worker->kw_dying = true;
	cv_broadcast(&worker->kw_cv);
	mutex_exit(&worker->kw_lock);

	/* Wait for the thread to exit. */
	kthread_join(worker->kw_lwp);

	KASSERT(worker->kw_current_work == NULL);
	while ((work = TAILQ_FIRST(&worker->kw_work_list)) != NULL) {
		kthread_remove_work(worker, work);
		if (work->worker == worker)
			work->worker = NULL;
	}

	cv_destroy(&worker->kw_cv);
	mutex_destroy(&worker->kw_lock);
	kmem_free(worker, sizeof(*worker));
}

/*
 * kthread_flush_worker(worker)
 *
 *	Wait until all currently queued work on the worker has
 *	finished executing.
 */
void
kthread_flush_worker(struct kthread_worker *worker)
{
	struct kthread_flush_work fwork;
	bool queued;

	kthread_flush_work_init(&fwork);
	queued = kthread_queue_work(worker, &fwork.kfw_work);
	KASSERT(queued);
	if (queued)
		kthread_flush_work_wait(&fwork);
	else
		kthread_flush_work_fini(&fwork);
}

/*
 * kthread_init_work(work, func)
 *
 *	Initialize a kthread_work item with the given callback function.
 */
void
kthread_init_work(struct kthread_work *work,
    void (*func)(struct kthread_work *))
{

	work->func = func;
	work->worker = NULL;
	kthread_work_entry_init(work);
	work->canceling = 0;
}

/*
 * kthread_queue_work(worker, work)
 *
 *	Queue a work item on the worker.  Return true if the work was
 *	queued, or false if it was already pending or is being cancelled.
 */
bool
kthread_queue_work(struct kthread_worker *worker, struct kthread_work *work)
{
	bool ret = false;

	mutex_enter(&worker->kw_lock);
	KASSERT(work->worker == NULL || work->worker == worker);
	if (!kthread_work_pending(work) && work->canceling == 0) {
		kthread_insert_work_tail(worker, work);
		ret = true;
	}
	mutex_exit(&worker->kw_lock);

	return ret;
}

/*
 * kthread_flush_work(work)
 *
 *	If the work is pending or executing, put a barrier immediately
 *	after that incarnation and wait for the barrier.  A later requeue
 *	does not extend the flush.
 */
void
kthread_flush_work(struct kthread_work *work)
{
	struct kthread_worker *worker = work->worker;
	struct kthread_flush_work fwork;
	bool noop = false;

	if (worker == NULL)
		return;

	kthread_flush_work_init(&fwork);

	mutex_enter(&worker->kw_lock);
	KASSERT(work->worker == worker);
	if (kthread_work_pending(work))
		kthread_insert_work_after(worker, work, &fwork.kfw_work);
	else if (worker->kw_current_work == work)
		kthread_insert_work_head(worker, &fwork.kfw_work);
	else
		noop = true;
	mutex_exit(&worker->kw_lock);

	if (!noop)
		kthread_flush_work_wait(&fwork);
	else
		kthread_flush_work_fini(&fwork);
}

/*
 * kthread_cancel_work_sync(work)
 *
 *	Cancel a pending work item and wait for it to finish if it is
 *	currently executing.  Return true if a pending incarnation was
 *	cancelled, or false if the work was not pending.
 */
bool
kthread_cancel_work_sync(struct kthread_work *work)
{
	struct kthread_worker *worker = work->worker;
	bool ret = false;

	if (worker == NULL)
		return false;

	mutex_enter(&worker->kw_lock);
	KASSERT(work->worker == worker);

	/* A running work may also have a pending, requeued incarnation. */
	if (kthread_work_pending(work)) {
		kthread_remove_work(worker, work);
		ret = true;
	}

	if (worker->kw_current_work != work) {
		mutex_exit(&worker->kw_lock);
		return ret;
	}

	/*
	 * Block self-requeue while the lock is dropped.  The counter permits
	 * concurrent callers to wait without accidentally enabling queueing.
	 */
	work->canceling++;
	mutex_exit(&worker->kw_lock);

	kthread_flush_work(work);

	mutex_enter(&worker->kw_lock);
	KASSERT(work->canceling > 0);
	work->canceling--;
	mutex_exit(&worker->kw_lock);

	return ret;
}

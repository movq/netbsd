/* SPDX-License-Identifier: BSD-2-Clause */
#include "taskq_shim.h"

int ncpu = 2;
unsigned test_allocations;
bool test_fail_alloc;
_Thread_local kthread_t *test_curthread;
_Thread_local bool *test_wait_observer;
static pthread_key_t tsd_key;
static struct threadpool pool;
static pthread_mutex_t workers_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t workers_cv = PTHREAD_COND_INITIALIZER;
static unsigned workers;

void
test_cv_wait(kcondvar_t *cv, kmutex_t *mtx)
{
	if (test_wait_observer != NULL)
		__atomic_store_n(test_wait_observer, true, __ATOMIC_RELEASE);
	VERIFY0(pthread_cond_wait(cv, mtx));
}

void *
kmem_alloc(size_t size, int flags)
{
	if (flags == KM_NOSLEEP && test_fail_alloc)
		return (NULL);
	void *p = malloc(size);
	assert(p != NULL);
	__atomic_add_fetch(&test_allocations, 1, __ATOMIC_RELAXED);
	return (p);
}

void *
kmem_zalloc(size_t size, int flags)
{
	void *p = kmem_alloc(size, flags);
	if (p != NULL)
		memset(p, 0, size);
	return (p);
}

void
kmem_free(void *p, size_t size)
{
	assert(p != NULL);
	memset(p, 0xa5, size);
	free(p);
	__atomic_sub_fetch(&test_allocations, 1, __ATOMIC_RELAXED);
}

void tsd_create(uint_t *k, void (*d)(void *))
{
	VERIFY0(pthread_key_create(&tsd_key, d));
	*k = 0;
}
void tsd_destroy(uint_t *k) { VERIFY0(pthread_key_delete(tsd_key)); }
void tsd_set(uint_t k, void *p) { VERIFY0(pthread_setspecific(tsd_key, p)); }
void *tsd_get(uint_t k) { return (pthread_getspecific(tsd_key)); }

int threadpool_get(struct threadpool **p, pri_t pri) { *p = &pool; return (0); }
void threadpool_put(struct threadpool *p, pri_t pri) {}

void
threadpool_job_init(struct threadpool_job *j,
    void (*fn)(struct threadpool_job *), kmutex_t *lock, const char *fmt, ...)
{
	memset(j, 0, sizeof (*j));
	j->func = fn;
	j->lock = lock;
}

struct launch {
	struct threadpool_job *job;
	void (*func)(struct threadpool_job *);
	kthread_t thread;
};

static void *
run_job(void *arg)
{
	struct launch *launch = arg;
	test_curthread = &launch->thread;
	launch->func(launch->job);
	/* No job accesses after job_done: the queue may already be freed. */
	assert(tsd_get(0) == NULL);
	free(launch);
	mutex_enter(&workers_lock);
	workers--;
	cv_broadcast(&workers_cv);
	mutex_exit(&workers_lock);
	return (NULL);
}

void
threadpool_schedule_job(struct threadpool *p, struct threadpool_job *j)
{
	assert(j->worker == NULL);
	struct launch *launch = calloc(1, sizeof (*launch));
	assert(launch != NULL);
	launch->job = j;
	launch->func = j->func;
	j->worker = &launch->thread;
	mutex_enter(&workers_lock);
	workers++;
	mutex_exit(&workers_lock);
	/*
	 * Use a local pthread_t: the worker can free launch before detach.
	 * j->lock, held by the caller, keeps job_done from racing this setup.
	 */
	pthread_t thread;
	VERIFY0(pthread_create(&thread, NULL, run_job, launch));
	launch->thread.thread = thread;
	VERIFY0(pthread_detach(thread));
}

void
threadpool_job_done(struct threadpool_job *j)
{
	assert(j->worker == test_curthread);
	j->worker = NULL;
}

void threadpool_job_destroy(struct threadpool_job *j) { assert(j->worker == NULL); }

void
test_workers_drain(void)
{
	mutex_enter(&workers_lock);
	while (workers != 0)
		cv_wait(&workers_cv, &workers_lock);
	mutex_exit(&workers_lock);
}

static pthread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t timer_cv = PTHREAD_COND_INITIALIZER;
static callout_t *timers;
static taskq_clock_t ticks;
static bool timer_paused;
static bool timer_entered;

taskq_clock_t ddi_get_lbolt(void) { return (__atomic_load_n(&ticks, __ATOMIC_RELAXED)); }
void test_clock_set(taskq_clock_t t) { __atomic_store_n(&ticks, t, __ATOMIC_RELAXED); }

void
callout_init(callout_t *c, unsigned flags)
{
	memset(c, 0, sizeof (*c));
	mutex_enter(&timer_lock);
	c->next = timers;
	timers = c;
	mutex_exit(&timer_lock);
}

void callout_setfunc(callout_t *c, void (*fn)(void *), void *arg)
{
	c->func = fn;
	c->arg = arg;
}

void
callout_schedule(callout_t *c, int delay)
{
	assert(delay > 0);
	mutex_enter(&timer_lock);
	c->deadline = ddi_get_lbolt() + delay;
	c->pending = true;
	mutex_exit(&timer_lock);
}

bool
callout_stop(callout_t *c)
{
	mutex_enter(&timer_lock);
	bool pending = c->pending;
	c->pending = false;
	mutex_exit(&timer_lock);
	return (pending);
}

bool
callout_halt(callout_t *c, void *interlock)
{
	assert(interlock == NULL);
	mutex_enter(&timer_lock);
	c->pending = false;
	bool running = c->running;
	while (c->running)
		cv_wait(&timer_cv, &timer_lock);
	mutex_exit(&timer_lock);
	return (running);
}

void
callout_destroy(callout_t *c)
{
	mutex_enter(&timer_lock);
	assert(!c->pending && !c->running);
	callout_t **p;
	for (p = &timers; *p != c; p = &(*p)->next)
		assert(*p != NULL);
	*p = c->next;
	mutex_exit(&timer_lock);
}

void
test_clock_advance(unsigned delta)
{
	__atomic_add_fetch(&ticks, delta, __ATOMIC_RELAXED);
	mutex_enter(&timer_lock);
	for (;;) {
		callout_t *c;
		for (c = timers; c != NULL; c = c->next)
			if (c->pending && (int)(ddi_get_lbolt() - c->deadline) >= 0)
				break;
		if (c == NULL)
			break;
		c->pending = false;
		c->running = true;
		timer_entered = true;
		cv_broadcast(&timer_cv);
		while (timer_paused)
			cv_wait(&timer_cv, &timer_lock);
		mutex_exit(&timer_lock);
		c->func(c->arg);
		mutex_enter(&timer_lock);
		c->running = false;
		cv_broadcast(&timer_cv);
	}
	mutex_exit(&timer_lock);
}

void
test_timer_pause(bool pause)
{
	mutex_enter(&timer_lock);
	timer_paused = pause;
	if (pause)
		timer_entered = false;
	cv_broadcast(&timer_cv);
	mutex_exit(&timer_lock);
}

void
test_timer_wait_running(void)
{
	mutex_enter(&timer_lock);
	while (!timer_entered)
		cv_wait(&timer_cv, &timer_lock);
	mutex_exit(&timer_lock);
}

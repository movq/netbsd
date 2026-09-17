/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Host test substitutes for NetBSD synchronization, threadpool and callouts.
 * The test builds the unmodified SPL taskq source and OpenZFS AVL source.
 */
#ifndef TASKQ_TEST_SHIM_H
#define	TASKQ_TEST_SHIM_H

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <time.h>
#include <unistd.h>

typedef unsigned int uint_t;
typedef unsigned long ulong_t;
typedef bool boolean_t;
typedef int pri_t;
typedef uint32_t taskq_clock_t;
#define	clock_t	taskq_clock_t
#define	B_TRUE	true
#define	B_FALSE	false
#define	ASSERT(x)	assert(x)
#define	VERIFY(x)	assert(x)
#define	ASSERT0(x)	assert((x) == 0)
#define	ASSERT0P(x)	assert((x) == NULL)
#define	ASSERT3P(a, op, b)	assert((a) op (b))
#define	ASSERT3U(a, op, b)	assert((a) op (b))
#define	ASSERT3S(a, op, b)	assert((a) op (b))
#define	VERIFY3P	ASSERT3P
#define	VERIFY3U	ASSERT3U
#define	VERIFY3S	ASSERT3S
#define	VERIFY0(x)	assert((x) == 0)
#define	CTASSERT(x)	_Static_assert(x, #x)
#define	MAX(a, b)	((a) > (b) ? (a) : (b))
#define	KM_SLEEP	0
#define	KM_NOSLEEP	1
#define	MUTEX_DEFAULT	0
#define	IPL_NONE	0
#define	CV_DEFAULT	0
#define	CALLOUT_MPSAFE	0
#define	minclsyspri	0
#define	EXPORT_SYMBOL(x)

typedef struct test_thread {
	pthread_t thread;
} kthread_t;
extern _Thread_local kthread_t *test_curthread;
#define	curthread	test_curthread
extern int ncpu;
struct proc;

typedef pthread_mutex_t kmutex_t;
typedef pthread_cond_t kcondvar_t;
#define	mutex_init(m, a, b, c)	VERIFY0(pthread_mutex_init(m, NULL))
#define	mutex_destroy(m)	VERIFY0(pthread_mutex_destroy(m))
#define	mutex_enter(m)		VERIFY0(pthread_mutex_lock(m))
#define	mutex_exit(m)		VERIFY0(pthread_mutex_unlock(m))
#define	MUTEX_HELD(m)		1
#define	cv_init(c, a, b, d)	VERIFY0(pthread_cond_init(c, NULL))
#define	cv_destroy(c)		VERIFY0(pthread_cond_destroy(c))
extern _Thread_local bool *test_wait_observer;
void test_cv_wait(kcondvar_t *, kmutex_t *);
#define	cv_wait(c, m)		test_cv_wait(c, m)
#define	cv_broadcast(c)		VERIFY0(pthread_cond_broadcast(c))

#define	atomic_store_release(p, v)	__atomic_store_n(p, v, __ATOMIC_RELEASE)
#define	atomic_load_acquire(p)		__atomic_load_n(p, __ATOMIC_ACQUIRE)
#define	atomic_inc_64_nv(p)		__atomic_add_fetch(p, 1, __ATOMIC_SEQ_CST)

extern unsigned test_allocations;
extern bool test_fail_alloc;
void *kmem_alloc(size_t, int);
void *kmem_zalloc(size_t, int);
void kmem_free(void *, size_t);
void tsd_create(uint_t *, void (*)(void *));
void tsd_destroy(uint_t *);
void tsd_set(uint_t, void *);
void *tsd_get(uint_t);

struct threadpool { unsigned unused; };
struct threadpool_job {
	kmutex_t *lock;
	void (*func)(struct threadpool_job *);
	kthread_t *worker;
};
int threadpool_get(struct threadpool **, pri_t);
void threadpool_put(struct threadpool *, pri_t);
void threadpool_job_init(struct threadpool_job *,
    void (*)(struct threadpool_job *), kmutex_t *, const char *, ...);
void threadpool_schedule_job(struct threadpool *, struct threadpool_job *);
void threadpool_job_done(struct threadpool_job *);
void threadpool_job_destroy(struct threadpool_job *);

typedef struct test_callout {
	struct test_callout *next;
	void (*func)(void *);
	void *arg;
	taskq_clock_t deadline;
	bool pending;
	bool running;
} callout_t;
void callout_init(callout_t *, unsigned);
void callout_setfunc(callout_t *, void (*)(void *), void *);
void callout_schedule(callout_t *, int);
bool callout_stop(callout_t *);
bool callout_halt(callout_t *, void *);
void callout_destroy(callout_t *);
taskq_clock_t ddi_get_lbolt(void);
void test_clock_set(taskq_clock_t);
void test_clock_advance(unsigned);
void test_workers_drain(void);
void test_timer_pause(bool);
void test_timer_wait_running(void);

#endif

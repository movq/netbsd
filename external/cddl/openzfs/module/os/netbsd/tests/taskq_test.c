/* SPDX-License-Identifier: BSD-2-Clause */
#include "taskq_shim.h"
#include <sys/taskq.h>

struct gate {
	pthread_mutex_t lock;
	pthread_cond_t cv;
	bool entered;
	bool released;
};

static void
gate_init(struct gate *g)
{
	memset(g, 0, sizeof (*g));
	mutex_init(&g->lock, 0, 0, 0);
	cv_init(&g->cv, 0, 0, 0);
}

static void
gate_fini(struct gate *g)
{
	cv_destroy(&g->cv);
	mutex_destroy(&g->lock);
}

static void
gate_entered(struct gate *g)
{
	mutex_enter(&g->lock);
	while (!g->entered)
		cv_wait(&g->cv, &g->lock);
	mutex_exit(&g->lock);
}

static void
gate_release(struct gate *g)
{
	mutex_enter(&g->lock);
	g->released = true;
	cv_broadcast(&g->cv);
	mutex_exit(&g->lock);
}

static void
block(void *arg)
{
	struct gate *g = arg;
	assert(taskq_member(taskq_of_curthread(), curthread));
	mutex_enter(&g->lock);
	g->entered = true;
	cv_broadcast(&g->cv);
	while (!g->released)
		cv_wait(&g->cv, &g->lock);
	mutex_exit(&g->lock);
}

static taskq_t *
queue(int n)
{
	taskq_t *tq = taskq_create("test", n, 0, 0, 0, TASKQ_DYNAMIC);
	assert(tq != NULL);
	return (tq);
}

static void increment(void *p) { __atomic_add_fetch((int *)p, 1, __ATOMIC_RELAXED); }
static void forbidden(void *p) { assert(!"cancelled callback ran"); }

enum wait_op { WAIT_ID, WAIT_ALL, WAIT_OUTSTANDING, CANCEL, DESTROY };
struct waiter {
	taskq_t *tq;
	taskqid_t id;
	enum wait_op op;
	int result;
	bool waiting;
	bool done;
	pthread_t thread;
};

static void *
waiter_run(void *arg)
{
	struct waiter *w = arg;
	test_wait_observer = &w->waiting;
	switch (w->op) {
	case WAIT_ID: taskq_wait_id(w->tq, w->id); break;
	case WAIT_ALL: taskq_wait(w->tq); break;
	case WAIT_OUTSTANDING: taskq_wait_outstanding(w->tq, w->id); break;
	case CANCEL: w->result = taskq_cancel_id(w->tq, w->id, B_TRUE); break;
	case DESTROY: taskq_destroy(w->tq); break;
	}
	__atomic_store_n(&w->done, true, __ATOMIC_RELEASE);
	return (NULL);
}

static void
waiter_start(struct waiter *w, taskq_t *tq, taskqid_t id, enum wait_op op)
{
	*w = (struct waiter){ .tq = tq, .id = id, .op = op };
	VERIFY0(pthread_create(&w->thread, NULL, waiter_run, w));
	while (!__atomic_load_n(&w->waiting, __ATOMIC_ACQUIRE)) {
		assert(!__atomic_load_n(&w->done, __ATOMIC_ACQUIRE));
		usleep(100);
	}
}

static void
waiter_join(struct waiter *w)
{
	VERIFY0(pthread_join(w->thread, NULL));
	assert(__atomic_load_n(&w->done, __ATOMIC_ACQUIRE));
}

static int sequence[8], seqpos;
static void record(void *arg) { sequence[seqpos++] = (int)(intptr_t)arg; }

static void
test_queued(void)
{
	struct gate g;
	gate_init(&g);
	taskq_t *tq = queue(1);
	taskqid_t busy = taskq_dispatch(tq, block, &g, TQ_SLEEP);
	gate_entered(&g);
	assert(taskq_cancel_id(tq, busy, B_FALSE) == EBUSY);
	taskqid_t cancelled = taskq_dispatch(tq, forbidden, NULL, TQ_SLEEP);
	assert(taskq_dispatch(tq, forbidden, NULL, TQ_NOQUEUE) == 0);
	assert(taskq_dispatch(tq, forbidden, NULL, TQ_NOALLOC) == 0);
	test_fail_alloc = true;
	assert(taskq_dispatch(tq, forbidden, NULL, TQ_NOSLEEP) == 0);
	test_fail_alloc = false;
	assert(taskq_cancel_id(tq, cancelled, B_TRUE) == 0);
	assert(taskq_cancel_id(tq, cancelled, B_TRUE) == ENOENT);
	taskq_dispatch(tq, record, (void *)1, TQ_SLEEP);
	taskq_dispatch(tq, record, (void *)2, TQ_FRONT);
	taskq_dispatch(tq, record, (void *)3, TQ_SLEEP);
	struct waiter w;
	waiter_start(&w, tq, busy, CANCEL);
	gate_release(&g);
	waiter_join(&w);
	assert(w.result == ENOENT);
	taskq_wait(tq);
	assert(seqpos == 3 && sequence[0] == 2 &&
	    sequence[1] == 1 && sequence[2] == 3);
	assert(taskq_cancel_id(tq, busy, B_TRUE) == ENOENT);
	taskq_wait_id(tq, 0);
	taskq_destroy(tq);
	gate_fini(&g);
}

static void
test_waits(void)
{
	struct gate a, b;
	gate_init(&a);
	gate_init(&b);
	taskq_t *tq = queue(2);
	taskqid_t id = taskq_dispatch(tq, block, &a, TQ_SLEEP);
	gate_entered(&a);
	struct waiter snapshot, single;
	waiter_start(&snapshot, tq, 0, WAIT_OUTSTANDING);
	/* Dispatched after the snapshot: neither waiter should wait for b. */
	taskq_dispatch(tq, block, &b, TQ_SLEEP);
	gate_entered(&b);
	waiter_start(&single, tq, id, WAIT_ID);
	gate_release(&a);
	waiter_join(&single);
	waiter_join(&snapshot);
	struct waiter all;
	waiter_start(&all, tq, 0, WAIT_ALL);
	gate_release(&b);
	waiter_join(&all);
	taskq_destroy(tq);
	gate_fini(&a);
	gate_fini(&b);
}

static void
test_delayed(void)
{
	taskq_t *tq = queue(1);
	int count = 0;
	test_clock_set(UINT32_MAX - 5);
	taskqid_t later = taskq_dispatch_delay(tq, increment, &count, 0,
	    ddi_get_lbolt() + 20);
	taskqid_t first = taskq_dispatch_delay(tq, increment, &count, 0,
	    ddi_get_lbolt() + 10);
	taskqid_t cancelled = taskq_dispatch_delay(tq, forbidden, NULL, 0,
	    ddi_get_lbolt() + 8);
	assert(taskq_cancel_id(tq, cancelled, B_FALSE) == 0);
	test_clock_advance(9);
	test_workers_drain();
	assert(count == 0);
	test_clock_advance(1);
	taskq_wait_id(tq, first);
	assert(count == 1);
	assert(taskq_cancel_id(tq, first, B_TRUE) == ENOENT);
	struct waiter outstanding;
	waiter_start(&outstanding, tq, later, WAIT_OUTSTANDING);
	test_clock_advance(10);
	waiter_join(&outstanding);
	assert(count == 2);
	taskq_dispatch_delay(tq, increment, &count, 0, ddi_get_lbolt() - 1);
	taskq_wait(tq);
	assert(count == 3);
	taskq_t *other = queue(1);
	taskqid_t otherid = taskq_dispatch_delay(other, forbidden, NULL, 0,
	    ddi_get_lbolt() + 1);
	assert(otherid != later);
	assert(taskq_cancel_id(tq, otherid, B_TRUE) == ENOENT);
	assert(taskq_cancel_id(other, otherid, B_TRUE) == 0);
	taskq_destroy(other);
	taskq_destroy(tq);
}

static void *
fire_timer(void *unused)
{
	test_clock_advance(1);
	return (NULL);
}

static void
test_timer_teardown(void)
{
	taskq_t *tq = queue(1);
	taskqid_t id = taskq_dispatch_delay(tq, forbidden, NULL, 0,
	    ddi_get_lbolt() + 1);
	/*
	 * Simulate a dequeued timer whose callback hasn't acquired tq_lock.
	 * Cancellation must succeed, and destruction must await that callback.
	 */
	test_timer_pause(true);
	pthread_t timer;
	VERIFY0(pthread_create(&timer, NULL, fire_timer, NULL));
	test_timer_wait_running();
	assert(taskq_cancel_id(tq, id, B_TRUE) == 0);
	struct waiter destroy;
	waiter_start(&destroy, tq, 0, DESTROY);
	test_timer_pause(false);
	VERIFY0(pthread_join(timer, NULL));
	waiter_join(&destroy);
}

struct embedded {
	taskq_ent_t ent;
	taskq_t *next;
	struct gate *first;
	struct gate *second;
};

static void
embedded_second(void *arg)
{
	struct embedded *e = arg;
	struct gate *g = e->second;
	assert(taskq_empty_ent(&e->ent));
	kmem_free(e, sizeof (*e));
	block(g);
}

static void
embedded_first(void *arg)
{
	struct embedded *e = arg;
	struct gate *first = e->first, *second = e->second;
	taskq_t *next = e->next;
	assert(taskq_empty_ent(&e->ent));
	taskq_dispatch_ent(next, embedded_second, e, 0, &e->ent);
	/* The other queue frees the entry before this callback returns. */
	gate_entered(second);
	block(first);
}

static void
test_embedded(void)
{
	taskq_t *a = queue(1), *b = queue(1);
	struct gate first, second, initial;
	gate_init(&first);
	gate_init(&second);
	gate_init(&initial);
	taskq_dispatch(a, block, &initial, 0);
	gate_entered(&initial);
	struct embedded *e = kmem_zalloc(sizeof (*e), KM_SLEEP);
	taskq_init_ent(&e->ent);
	e->first = &first;
	e->second = &second;
	e->next = b;
	taskq_dispatch_ent(a, embedded_first, e, 0, &e->ent);
	assert(!taskq_empty_ent(&e->ent));
	taskqid_t id = e->ent.tqent_id;
	gate_release(&initial);
	gate_entered(&first);
	struct waiter w;
	waiter_start(&w, a, id, WAIT_ID);
	gate_release(&first);
	waiter_join(&w);
	gate_release(&second);
	taskq_wait(b);
	taskq_destroy(a);
	taskq_destroy(b);
	gate_fini(&first);
	gate_fini(&second);
	gate_fini(&initial);
}

static void
test_synced(void)
{
	kthread_t **threads;
	taskq_t *tq = taskq_create_synced("synced", 3, 0, 0, 0,
	    TASKQ_PREPOPULATE, &threads);
	assert(tq != NULL && threads != NULL);
	for (int i = 0; i < 3; i++) {
		assert(taskq_member(tq, threads[i]));
		for (int j = 0; j < i; j++)
			assert(threads[i] != threads[j]);
	}
	struct gate gates[3];
	for (int i = 0; i < 3; i++) {
		gate_init(&gates[i]);
		taskq_dispatch(tq, block, &gates[i], 0);
	}
	for (int i = 0; i < 3; i++)
		gate_entered(&gates[i]);
	for (int i = 0; i < 3; i++)
		gate_release(&gates[i]);
	taskq_wait(tq);
	for (int i = 0; i < 3; i++) {
		assert(taskq_member(tq, threads[i]));
		gate_fini(&gates[i]);
	}
	kmem_free(threads, sizeof (*threads) * 3);
	taskq_destroy(tq);
}

static void
test_stress(void)
{
	taskq_t *tq = queue(4);
	int count = 0, cancelled = 0;
	for (int i = 0; i < 4000; i++) {
		taskqid_t id = taskq_dispatch(tq, increment, &count,
		    i % 2 ? TQ_FRONT : TQ_SLEEP);
		int result = taskq_cancel_id(tq, id, i % 2);
		assert(result == 0 || result == ENOENT || result == EBUSY);
		if (result == 0)
			cancelled++;
	}
	taskq_wait(tq);
	assert(count + cancelled == 4000);
	taskq_destroy(tq);
}

static void *
timer_stress(void *unused)
{
	for (int i = 0; i < 1000; i++) {
		test_clock_advance(1);
		usleep(10);
	}
	return (NULL);
}

static void
test_delayed_stress(void)
{
	taskq_t *tq = queue(4);
	int count = 0, cancelled = 0;
	pthread_t timer;
	VERIFY0(pthread_create(&timer, NULL, timer_stress, NULL));
	for (int i = 0; i < 2000; i++) {
		taskqid_t id = taskq_dispatch_delay(tq, increment, &count, 0,
		    ddi_get_lbolt() + 1);
		int result = taskq_cancel_id(tq, id, B_FALSE);
		assert(result == 0 || result == ENOENT || result == EBUSY);
		if (result == 0)
			cancelled++;
	}
	VERIFY0(pthread_join(timer, NULL));
	test_clock_advance(2);
	taskq_wait(tq);
	assert(count + cancelled == 2000);
	taskq_destroy(tq);
}

int
main(void)
{
	alarm(45);
	taskq_init();
	assert(taskq_of_curthread() == NULL);
	test_queued();
	test_waits();
	test_delayed();
	test_timer_teardown();
	test_embedded();
	test_synced();
	test_stress();
	test_delayed_stress();
	test_workers_drain();
	taskq_fini();
	test_workers_drain();
	assert(test_allocations == 0);
	puts("taskq: ordering, cancellation, waits, timers, entry lifetime, "
	    "synced workers and stress tests passed");
	return (0);
}

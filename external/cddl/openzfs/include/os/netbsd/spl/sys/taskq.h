/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_TASKQ_H_
#define	_NETBSD_SPL_TASKQ_H_

#include <sys/types.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/avl.h>

typedef struct taskq taskq_t;
typedef uint64_t taskqid_t;
typedef void (task_func_t)(void *);

/*
 * Caller-owned entries may be freed or dispatched again by their callback.
 * The executor must stop accessing the entry before invoking that callback.
 */
typedef struct openzfs_taskq_ent {
	TAILQ_ENTRY(openzfs_taskq_ent) tqent_list;
	avl_node_t	tqent_node;
	task_func_t	*tqent_func;
	void		*tqent_arg;
	taskqid_t	tqent_id;
	clock_t		tqent_expire;
	unsigned	tqent_queued;
	boolean_t	tqent_dynamic;
	boolean_t	tqent_delayed;
} taskq_ent_t;

#define	TASKQ_NAMELEN	31
#define	TASKQ_PREPOPULATE	0x0001
#define	TASKQ_CPR_SAFE		0x0002
#define	TASKQ_DYNAMIC		0x0004
#define	TASKQ_THREADS_CPU_PCT	0x0008
#define	TASKQ_DC_BATCH		0x0010

#define	TQ_SLEEP	0x00
#define	TQ_NOSLEEP	0x01
#define	TQ_NOQUEUE	0x02
#define	TQ_NOALLOC	0x04
#define	TQ_FRONT	0x08
#define	TASKQID_INVALID	((taskqid_t)0)

#define	system_taskq		openzfs_system_taskq
#define	system_delay_taskq	openzfs_system_delay_taskq
#define	taskq_init		openzfs_taskq_init
#define	taskq_fini		openzfs_taskq_fini
#define	taskq_create		openzfs_taskq_create
#define	taskq_create_proc	openzfs_taskq_create_proc
#define	taskq_create_synced	openzfs_taskq_create_synced
#define	taskq_destroy		openzfs_taskq_destroy
#define	taskq_dispatch		openzfs_taskq_dispatch
#define	taskq_dispatch_delay	openzfs_taskq_dispatch_delay
#define	taskq_dispatch_ent	openzfs_taskq_dispatch_ent
#define	taskq_init_ent		openzfs_taskq_init_ent
#define	taskq_empty_ent		openzfs_taskq_empty_ent
#define	taskq_wait		openzfs_taskq_wait
#define	taskq_wait_id		openzfs_taskq_wait_id
#define	taskq_wait_outstanding	openzfs_taskq_wait_outstanding
#define	taskq_cancel_id		openzfs_taskq_cancel_id
#define	taskq_member		openzfs_taskq_member
#define	taskq_of_curthread	openzfs_taskq_of_curthread

extern taskq_t *system_taskq;
extern taskq_t *system_delay_taskq;

/* Initialize before creating any queues; finalize after destroying them. */
void taskq_init(void);
void taskq_fini(void);
taskq_t *taskq_create(const char *, int, pri_t, int, int, uint_t);
taskq_t *taskq_create_proc(const char *, int, pri_t, int, int,
    struct proc *, uint_t);
taskq_t *taskq_create_synced(const char *, int, pri_t, int, int, uint_t,
    kthread_t ***);
void taskq_destroy(taskq_t *);
taskqid_t taskq_dispatch(taskq_t *, task_func_t, void *, uint_t);
taskqid_t taskq_dispatch_delay(taskq_t *, task_func_t, void *, uint_t, clock_t);
void taskq_dispatch_ent(taskq_t *, task_func_t, void *, uint_t, taskq_ent_t *);
void taskq_init_ent(taskq_ent_t *);
int taskq_empty_ent(taskq_ent_t *);
void taskq_wait(taskq_t *);
void taskq_wait_id(taskq_t *, taskqid_t);
void taskq_wait_outstanding(taskq_t *, taskqid_t);
int taskq_cancel_id(taskq_t *, taskqid_t, boolean_t);
int taskq_member(taskq_t *, kthread_t *);
taskq_t *taskq_of_curthread(void);

#endif

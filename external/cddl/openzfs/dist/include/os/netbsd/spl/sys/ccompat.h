/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_CCOMPAT_H_
#define	_NETBSD_SPL_CCOMPAT_H_

#include <sys/types.h>
#include <sys/atomic.h>

/* Common zvol hash chains are protected by zvol_state_lock. */
struct hlist_node {
	struct hlist_node *next, **pprev;
};
struct hlist_head {
	struct hlist_node *first;
};

#define	hlist_for_each(p, head)	\
	for ((p) = (head)->first; (p) != NULL; (p) = (p)->next)
#define	hlist_entry(p, type, member)	container_of(p, type, member)
#define	INIT_HLIST_HEAD(head)	((head)->first = NULL)
#define	INIT_HLIST_NODE(node) do {	\
	(node)->next = NULL;		\
	(node)->pprev = NULL;		\
} while (0)

static inline void
hlist_add_head(struct hlist_node *node, struct hlist_head *head)
{
	node->next = head->first;
	if (head->first != NULL)
		head->first->pprev = &node->next;
	head->first = node;
	node->pprev = &head->first;
}

static inline void
hlist_del(struct hlist_node *node)
{
	*node->pprev = node->next;
	if (node->next != NULL)
		node->next->pprev = node->pprev;
}

typedef struct {
	volatile unsigned counter;
} atomic_t;

static inline int
atomic_read(const atomic_t *value)
{
	return ((int)atomic_load_relaxed(&value->counter));
}

static inline int
atomic_inc(atomic_t *value)
{
	return ((int)atomic_inc_uint_nv(&value->counter));
}

static inline int
atomic_dec(atomic_t *value)
{
	return ((int)atomic_dec_uint_nv(&value->counter));
}

#endif

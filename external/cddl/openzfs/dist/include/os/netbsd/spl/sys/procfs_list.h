/* SPDX-License-Identifier: CDDL-1.0 */
/*
 * Copyright (c) 2018 by Delphix. All rights reserved.
 *
 * Interface adapted from the OpenZFS FreeBSD SPL.
 */
#ifndef _NETBSD_SPL_PROCFS_LIST_H_
#define	_NETBSD_SPL_PROCFS_LIST_H_

#include <sys/list.h>
#include <sys/mutex.h>
#include <sys/kstat.h>

typedef struct procfs_list procfs_list_t;
struct procfs_list {
	void		*pl_private;
	void		*pl_next_data;
	kstat_t		*pl_kstat;
	kmutex_t	pl_lock;
	list_t		pl_list;
	uint64_t	pl_next_id;
	int		(*pl_show)(struct seq_file *, void *);
	int		(*pl_show_header)(struct seq_file *);
	int		(*pl_clear)(procfs_list_t *);
	size_t		pl_node_offset;
};

typedef struct procfs_list_node {
	list_node_t	pln_link;
	uint64_t	pln_id;
} procfs_list_node_t;

#define	procfs_list_install	openzfs_procfs_list_install
#define	procfs_list_uninstall	openzfs_procfs_list_uninstall
#define	procfs_list_destroy	openzfs_procfs_list_destroy
#define	procfs_list_add		openzfs_procfs_list_add

void procfs_list_install(const char *, const char *, const char *, mode_t,
    procfs_list_t *, int (*)(struct seq_file *, void *),
    int (*)(struct seq_file *), int (*)(procfs_list_t *), size_t);
void procfs_list_uninstall(procfs_list_t *);
void procfs_list_destroy(procfs_list_t *);
void procfs_list_add(procfs_list_t *, void *);

#endif

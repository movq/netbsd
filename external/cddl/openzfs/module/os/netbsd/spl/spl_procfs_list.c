/* SPDX-License-Identifier: BSD-2-Clause */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/procfs_list.h>

static void *
procfs_list_addr(kstat_t *ksp, loff_t index)
{
	procfs_list_t *pl = ksp->ks_private;

	KASSERT(MUTEX_HELD(&pl->pl_lock));
	pl->pl_next_data = index == 0 ? list_head(&pl->pl_list) :
	    list_next(&pl->pl_list, pl->pl_next_data);
	return (pl->pl_next_data != NULL ? pl : NULL);
}

static int
procfs_list_data(char *buf, size_t size, void *data)
{
	procfs_list_t *pl = data;
	struct seq_file f = { .sf_buf = buf, .sf_size = size };
	int error;

	KASSERT(MUTEX_HELD(&pl->pl_lock));
	error = pl->pl_show(&f, pl->pl_next_data);
	return (error != 0 ? error : f.sf_error);
}

void
procfs_list_install(const char *module, const char *submodule,
    const char *name, mode_t mode, procfs_list_t *pl,
    int (*show)(struct seq_file *, void *),
    int (*show_header)(struct seq_file *),
    int (*clear)(procfs_list_t *), size_t node_offset)
{
	mutex_init(&pl->pl_lock, NULL, MUTEX_DEFAULT, NULL);
	list_create(&pl->pl_list, node_offset + sizeof (procfs_list_node_t),
	    node_offset + offsetof(procfs_list_node_t, pln_link));
	pl->pl_next_id = 1;
	pl->pl_node_offset = node_offset;
	pl->pl_next_data = NULL;
	pl->pl_show = show;
	pl->pl_show_header = show_header;
	pl->pl_clear = clear;
	/* pl_private belongs to the caller, including during installation. */
	pl->pl_kstat = kstat_create(module, 0, name, submodule,
	    KSTAT_TYPE_RAW, 0, KSTAT_FLAG_VIRTUAL);
	if (pl->pl_kstat != NULL) {
		kstat_t *ksp = pl->pl_kstat;
		ksp->ks_private = pl;
		ksp->ks_lock = &pl->pl_lock;
		ksp->ks_ndata = UINT32_MAX;
		/* Exports are read-only; preserve owner-only read permissions. */
		if ((mode & 0044) == 0)
			ksp->ks_sysctl_flags = CTLFLAG_PRIVATE;
		kstat_set_seq_raw_ops(ksp, show_header,
		    procfs_list_data, procfs_list_addr);
		kstat_install(ksp);
	}
}

void
procfs_list_uninstall(procfs_list_t *pl)
{
	/* Drain sysctl readers before the caller frees the list's records. */
	kstat_delete(pl->pl_kstat);
	pl->pl_kstat = NULL;
	pl->pl_next_data = NULL;
}

void
procfs_list_destroy(procfs_list_t *pl)
{
	KASSERT(pl->pl_kstat == NULL);
	KASSERT(list_is_empty(&pl->pl_list));
	list_destroy(&pl->pl_list);
	mutex_destroy(&pl->pl_lock);
}

void
procfs_list_add(procfs_list_t *pl, void *data)
{
	procfs_list_node_t *node = (void *)((char *)data + pl->pl_node_offset);

	KASSERT(MUTEX_HELD(&pl->pl_lock));
	node->pln_id = pl->pl_next_id++;
	list_insert_tail(&pl->pl_list, data);
}

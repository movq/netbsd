/*	$NetBSD$	*/

#include "linux/kernel.h"
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/errno.h>

#include <linux/err.h>
#include <linux/export.h>

#include <drm/ttm/ttm_backup.h>

void
ttm_backup_drop(struct file *backup, pgoff_t handle)
{
	STUB();
}

int
ttm_backup_copy_page(struct file *backup, struct page *dst, pgoff_t handle,
    bool intr)
{
	STUB();
	return -EOPNOTSUPP;
}

s64
ttm_backup_backup_page(struct file *backup, struct page *page, bool writeback,
    pgoff_t idx, gfp_t page_gfp, gfp_t alloc_gfp)
{
	STUB();
	return -EOPNOTSUPP;
}

void
ttm_backup_fini(struct file *backup)
{
}

u64
ttm_backup_bytes_avail(void)
{
	STUB();
	return 0;
}
EXPORT_SYMBOL_GPL(ttm_backup_bytes_avail);

struct file *
ttm_backup_shmem_create(loff_t size)
{
	STUB();
	return ERR_PTR(-EOPNOTSUPP);
}

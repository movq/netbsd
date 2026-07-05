/*	$NetBSD$	*/

/* Public domain. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/systm.h>

#include <linux/device.h>

struct linux_device_drvdata {
	struct device			*ldd_dev;
	void				*ldd_data;
	LIST_ENTRY(linux_device_drvdata)	ldd_entry;
};

static struct {
	kmutex_t lock;
	LIST_HEAD(, linux_device_drvdata) list;
} linux_device_drvdata;

void
linux_device_init(void)
{

	mutex_init(&linux_device_drvdata.lock, MUTEX_DEFAULT, IPL_HIGH);
	LIST_INIT(&linux_device_drvdata.list);
}

void
linux_device_fini(void)
{

	KASSERT(LIST_EMPTY(&linux_device_drvdata.list));
	mutex_destroy(&linux_device_drvdata.lock);
}

void
dev_set_drvdata(struct device *dev, void *data)
{
	struct linux_device_drvdata *entry, *newentry = NULL;

	KASSERT(dev != NULL);

	/*
	 * Allocate before taking the spin mutex.  It is harmless to allocate
	 * speculatively because an existing entry can be updated in place.
	 */
	if (data != NULL) {
		newentry = kmem_alloc(sizeof(*newentry), KM_SLEEP);
		newentry->ldd_dev = dev;
		newentry->ldd_data = data;
	}

	mutex_enter(&linux_device_drvdata.lock);
	LIST_FOREACH(entry, &linux_device_drvdata.list, ldd_entry) {
		if (entry->ldd_dev == dev)
			break;
	}

	if (entry != NULL) {
		if (data == NULL)
			LIST_REMOVE(entry, ldd_entry);
		else
			entry->ldd_data = data;
	} else if (newentry != NULL) {
		LIST_INSERT_HEAD(&linux_device_drvdata.list, newentry,
		    ldd_entry);
		newentry = NULL;
	}
	mutex_exit(&linux_device_drvdata.lock);

	if (entry != NULL && data == NULL)
		kmem_free(entry, sizeof(*entry));
	if (newentry != NULL)
		kmem_free(newentry, sizeof(*newentry));
}

void *
dev_get_drvdata(struct device *dev)
{
	struct linux_device_drvdata *entry;
	void *data = NULL;

	KASSERT(dev != NULL);

	mutex_enter(&linux_device_drvdata.lock);
	LIST_FOREACH(entry, &linux_device_drvdata.list, ldd_entry) {
		if (entry->ldd_dev == dev) {
			data = entry->ldd_data;
			break;
		}
	}
	mutex_exit(&linux_device_drvdata.lock);

	return data;
}

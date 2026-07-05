/* Public domain. */

#ifndef _LINUX_PROPERTY_H
#define _LINUX_PROPERTY_H

#include <linux/err.h>
#include <linux/fwnode.h>

static inline void
fwnode_handle_put(struct fwnode_handle *h)
{
}

static inline const struct fwnode_handle *
dev_fwnode(struct device *d)
{
	return NULL;
}

static inline bool
fwnode_device_is_available(const struct fwnode_handle *fwnode)
{
	return false;
}

static inline bool
device_property_present(const struct device *dev, const char *propname)
{
	return false;
}

static inline struct fwnode_handle *
fwnode_find_reference(const struct fwnode_handle *fwnode, const char *name,
    unsigned int index)
{
	return ERR_PTR(-ENOENT);
}

#endif

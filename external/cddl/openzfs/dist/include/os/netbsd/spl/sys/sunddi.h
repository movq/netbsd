/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_SUNDDI_H_
#define	_NETBSD_SPL_SUNDDI_H_

#include <sys/systm.h>
#include <sys/u8_textprep.h>

/*
 * These conversions are supplied by the existing NetBSD Solaris layer.
 * Do not include its sunddi.h: its sysevent interface uses the old event
 * representation, which differs from OpenZFS's nvlist-based representation.
 */
int ddi_strtol(const char *, char **, int, long *);
int ddi_strtoul(const char *, char **, int, unsigned long *);
int ddi_strtoull(const char *, char **, int, unsigned long long *);

/* Native /dev/zvol node creation supplied by the existing Solaris module. */
typedef struct dev_info {
	int di_cmajor;
	int di_bmajor;
} dev_info_t;
int ddi_create_minor_node(dev_info_t *, char *, int, minor_t, char *, int);
void ddi_remove_minor_node(dev_info_t *, char *);
#define	DDI_PSEUDO	""

/* Match the FreeBSD adapter; native strtoll supplies saturation and endptr. */
static inline int
ddi_strtoll(const char *str, char **endptr, int base, long long *result)
{
	*result = strtoll(str, endptr, base);
	return (0);
}

#define	ddi_copyin(from, to, size, flag) \
	ioctl_copyin((flag), (from), (to), (size))
#define	ddi_copyout(from, to, size, flag) \
	ioctl_copyout((flag), (from), (to), (size))

#endif

/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_ZVOL_OS_H_
#define	_NETBSD_ZVOL_OS_H_

#include <sys/types.h>

struct buf;
struct lwp;
struct uio;

int zvol_open(dev_t, int, int, struct lwp *);
int zvol_close(dev_t, int, int, struct lwp *);
int zvol_read(dev_t, struct uio *, int);
int zvol_write(dev_t, struct uio *, int);
int zvol_ioctl(dev_t, u_long, void *, int, struct lwp *);
void zvol_strategy(struct buf *);

#endif

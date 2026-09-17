/* SPDX-License-Identifier: BSD-2-Clause */
#include <sys/types.h>
#include <sys/sysctl.h>
#include <limits.h>
#include <string.h>
#include "../../libspl_impl.h"

ssize_t
getexecname_impl(char *buf)
{
	int mib[] = { CTL_KERN, KERN_PROC_ARGS, -1, KERN_PROC_PATHNAME };
	size_t len = PATH_MAX;

	if (sysctl(mib, __arraycount(mib), buf, &len, NULL, 0) == -1)
		return (-1);
	return (strnlen(buf, len));
}

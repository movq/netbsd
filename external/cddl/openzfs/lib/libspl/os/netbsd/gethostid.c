/* SPDX-License-Identifier: BSD-2-Clause */
#include <sys/types.h>
#include <sys/systeminfo.h>
#include <unistd.h>

unsigned long
get_system_hostid(void)
{
	/* Match the kernel adapter's uint32_t conversion of the native hostid. */
	return ((uint32_t)gethostid());
}

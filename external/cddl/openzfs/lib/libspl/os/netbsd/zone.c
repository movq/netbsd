/* SPDX-License-Identifier: BSD-2-Clause */
#include <zone.h>

zoneid_t
getzoneid(void)
{
	return (GLOBAL_ZONEID);
}

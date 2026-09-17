/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_RANDOM_H_
#define	_NETBSD_SPL_RANDOM_H_

#include_next <sys/random.h>
#include <sys/debug.h>

static inline uint32_t
random_in_range(uint32_t range)
{
	uint32_t value, threshold;

	VERIFY3U(range, !=, 0);
	threshold = -range % range;
	do {
		value = cprng_fast32();
	} while (value < threshold);
	return (value % range);
}

#endif

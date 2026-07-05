/*	$NetBSD$	*/

/* Public domain. */

#ifndef _LINUX_WORDPART_H_
#define _LINUX_WORDPART_H_

#include <linux/types.h>

#ifndef upper_32_bits
#define	upper_32_bits(n)	((u32)(((n) >> 16) >> 16))
#endif
#ifndef lower_32_bits
#define	lower_32_bits(n)	((u32)((n) & 0xffffffffU))
#endif

#define	upper_16_bits(n)	((u16)((n) >> 16))
#define	lower_16_bits(n)	((u16)((n) & 0xffffU))

#define	REPEAT_BYTE(x)		((~0UL / 0xff) * (x))
#define	REPEAT_BYTE_U32(x)	lower_32_bits(REPEAT_BYTE(x))

#endif	/* _LINUX_WORDPART_H_ */

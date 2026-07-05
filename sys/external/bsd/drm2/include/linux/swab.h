/* Public domain. */

#ifndef _LINUX_SWAB_H
#define _LINUX_SWAB_H

#include <sys/bswap.h>

#define	swab16(x)	bswap16(x)
#define	swab32(x)	bswap32(x)
#define	swab64(x)	bswap64(x)

#endif

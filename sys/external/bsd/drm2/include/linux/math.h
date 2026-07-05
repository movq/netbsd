#ifndef _LINUX_MATH_H
#define _LINUX_MATH_H

#include <linux/kernel.h>

#define	mult_frac(x, n, d) ({						\
	__typeof__(x) __x = (x);						\
	__typeof__(n) __n = (n);						\
	__typeof__(d) __d = (d);						\
	__typeof__(x) __q = __x / __d;					\
	__typeof__(x) __r = __x % __d;					\
	__q * __n + __r * __n / __d;					\
})

#endif

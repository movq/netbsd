/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_CCOMPILE_H_
#define	_NETBSD_SPL_CCOMPILE_H_

#include <sys/cdefs.h>
#include_next <sys/ccompile.h>

#define	likely(x)	__predict_true(x)
#define	unlikely(x)	__predict_false(x)
#define	noinline	__noinline
#define	__maybe_unused	__unused
#define	__init
#define	__exit

/* NetBSD's module loader uses the ELF symbol table. */
#define	EXPORT_SYMBOL(x)
#define	EXPORT_SYMBOL_GPL(x)
#define	MODULE_AUTHOR(x)
#define	MODULE_DESCRIPTION(x)
#define	MODULE_LICENSE(x)
#define	MODULE_VERSION(x)

#endif

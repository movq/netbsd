/*	$NetBSD$	*/

/* Public domain. */

#ifndef _LINUX_PGTABLE_H_
#define _LINUX_PGTABLE_H_

#include <uvm/uvm_extern.h>

#include <linux/mm_types.h>

#define	PAGE_KERNEL	UVM_PROT_RW
#define	PAGE_KERNEL_IO	UVM_PROT_RW

static inline pgprot_t
pgprot_writecombine(pgprot_t prot)
{

	return (prot & ~PMAP_CACHE_MASK) | PMAP_WRITE_COMBINE;
}

static inline pgprot_t
pgprot_noncached(pgprot_t prot)
{

	return (prot & ~PMAP_CACHE_MASK) | PMAP_NOCACHE;
}

static inline pgprot_t
pgprot_decrypted(pgprot_t prot)
{

	return prot;
}

#endif	/* _LINUX_PGTABLE_H_ */

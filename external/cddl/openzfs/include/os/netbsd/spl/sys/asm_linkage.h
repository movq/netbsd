/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _NETBSD_SPL_ASM_LINKAGE_H_
#define	_NETBSD_SPL_ASM_LINKAGE_H_

/* Only portable C checksum implementations are enabled during bring-up. */
#ifdef _ASM
#error "OpenZFS assembly linkage is not implemented on NetBSD"
#endif
#define	ASMABI

#endif

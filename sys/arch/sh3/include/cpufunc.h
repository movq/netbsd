/*	$NetBSD: cpufunc.h,v 1.13 2002/02/22 19:44:02 uch Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH3_CPUFUNC_H_
#define	_SH3_CPUFUNC_H_

#ifdef _KERNEL
#ifndef _LOCORE

/*
 * Memory-mapped register access method.
 */
#define	_reg_read_1(a)		(*(__volatile__ u_int8_t *)((vaddr_t)(a)))
#define	_reg_read_2(a)		(*(__volatile__ u_int16_t *)((vaddr_t)(a)))
#define	_reg_read_4(a)		(*(__volatile__ u_int32_t *)((vaddr_t)(a)))
#define	_reg_write_1(a, v)						\
	(*(__volatile__ u_int8_t *)(a) = (u_int8_t)(v))
#define	_reg_write_2(a, v)						\
	(*(__volatile__ u_int16_t *)(a) = (u_int16_t)(v))
#define	_reg_write_4(a, v)						\
	(*(__volatile__ u_int32_t *)(a) = (u_int32_t)(v))

/*
 * CPU exception/interrupt ops.	(locore_subr.S)
 */
/* suspend/resume external interrupt (SR.IMASK) */
u_int32_t _cpu_intr_suspend(void);
void _cpu_intr_resume(u_int32_t);
/* suspend/resume exception (SR.BL) */
u_int32_t _cpu_exception_suspend(void);
void _cpu_exception_resume(u_int32_t);

/* for delay loop. */
void _cpu_spin(u_int32_t);

/* Soft reset */
void cpu_reset(void);

#endif /* !_LOCORE */
#endif /* _KERNEL */
#endif /* !_SH3_CPUFUNC_H_ */

/*      $NetBSD: cpu_machdep.c,v 1.3 1994/11/25 19:09:50 ragge Exp $      */

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /* All bugs are subject to removal without further notice */
		

#include "machine/cpu.h"
#include "machine/sid.h"
#include "machine/param.h"
#include "machine/loconf.h"
#include "sys/types.h"
#include "machine/param.h"
#include "machine/vmparam.h"
#include "vm/vm.h"

int	cpu_notsupp(),cpu_notgen();
#ifdef	VAX750
int	v750_loinit();
#endif

struct	cpu_dep	cpu_calls[VAX_MAX+1]={
		/* Type 0,noexist */
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#ifdef	VAX780	/* Type 1, 11/{780,782,785} */
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#else
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#endif
#ifdef  VAX750	/* Type 2, 11/750 */
        v750_loinit,cpu_notgen,cpu_notgen,cpu_notgen,
#else
        cpu_notgen,cpu_notgen,cpu_notgen,cpu_notgen,
#endif
#ifdef	VAX730	/* Type 3, 11/{730,725}, ceauciesco-vax */
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#else
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#endif
#ifdef	VAX8600	/* Type 4, 8600/8650 (11/{790,795}) */
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#else
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#endif
#ifdef	VAX8200	/* Type 5, 8200, 8300, 8350 */
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#else
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#endif
#ifdef	VAX8800	/* Type 6, 85X0, 8700, 88X0 */
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#else
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#endif
#ifdef	VAX610	/* Type 7, KA610 */
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#else
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#endif
#ifdef	VAX630	/* Type 8, KA630 (uVAX II) */
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#else
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#endif
		/* Type 9, not used */
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#ifdef	VAX650  /* Type 10, KA65X (uVAX III) */
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#else
	cpu_notsupp,cpu_notsupp,cpu_notsupp,cpu_notsupp,
#endif
};

cpu_notgen()
{
	conout("This cputype not generated.\n");
	asm("halt");
}
cpu_notsupp()
{
	conout("This cputype not supported.\n");
	asm("halt");
}

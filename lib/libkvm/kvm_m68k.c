/*	$NetBSD: kvm_m68k.c,v 1.10 1997/03/21 18:44:23 gwr Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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

/*
 * Run-time kvm dispatcher for m68k machines.
 * The actual MD code is in the files:
 * kvm_m68k_cmn.c kvm_sun3.c ...
 *
 * Note: This file has to build on ALL m68k machines,
 * so do NOT include any <machine/*.h> files here.
 */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <limits.h>
#include <nlist.h>
#include <kvm.h>
#include <db.h>

#include "kvm_private.h"
#include "kvm_m68k.h"

/* Could put this in struct vmstate, but this is easier. */
static struct kvm_ops *ops;

struct name_ops {
	char *name;
	struct kvm_ops *ops;
};

static struct name_ops optbl[] = {
	{ "amiga", &_kvm_ops_cmn },
	{ "atari", &_kvm_ops_cmn },
	{ "sun3",  &_kvm_ops_sun3 },
	{ "sun3x",  &_kvm_ops_sun3x },
	{ NULL, NULL },
};


/*
 * Prepare for translation of kernel virtual addresses into offsets
 * into crash dump files.  This is where we do the dispatch work.
 */
int
_kvm_initvtop(kd)
	kvm_t *kd;
{
	char machine[256];
	int mib[2], len, rval;
	struct name_ops *nop;

	/* Which set of kvm functions should we use? */
	mib[0] = CTL_HW;
	mib[1] = HW_MACHINE;
	len = sizeof(machine);
	if (sysctl(mib, 2, machine, &len, NULL, 0) == -1)
		return (-1);

	for (nop = optbl; nop->name; nop++)
		if (!strcmp(machine, nop->name))
			goto found;
	_kvm_err(kd, 0, "%s: unknown machine!", machine);
	return (-1);

found:
	ops = nop->ops;
	return ((ops->initvtop)(kd));
}

void
_kvm_freevtop(kd)
	kvm_t *kd;
{
	(ops->freevtop)(kd);
}

int
_kvm_kvatop(kd, va, pap)
	kvm_t *kd;
	u_long va;
	u_long *pap;
{
	return ((ops->kvatop)(kd, va, pap));
}

off_t
_kvm_pa2off(kd, pa)
	kvm_t	*kd;
	u_long	pa;
{
	return ((ops->pa2off)(kd, pa));
}

/*	$NetBSD: darwin_signal.h,v 1.2 2002/11/26 23:54:10 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#ifndef	_DARWIN_DARWIN_H_
#define	_DARWIN_DARWIN_H_

#define DARWIN_SA_USERTRAMP 0x0100

union darwin_sigval {
	int sigval_int;
	void *sigval_ptr;
};

typedef struct darwin___siginfo {
	int si_signo;
	int si_errno;
	int si_code;
	int si_pid;
	unsigned int si_uid;
	int si_status;
	void *si_addr;
	union darwin_sigval si_value;
	long si_band;
	int pad[7];
} darwin_siginfo_t;

struct darwin_ucontext {
	int uc_onstack;
	sigset13_t uc_sigmask;
	stack_t uc_stack;
	struct darwin_ucontext *uc_link;
	size_t uc_mcsize;
	struct darwin_mcontext *uc_mcontext;
};

struct darwin___sigaction {
	union {
		void (*__sa_handler)(int);
		void (*__sa_sigaction)(int, struct darwin___siginfo *, void *);
	} sa_handler;
	void (*sa_tramp)(void *, int, int, darwin_siginfo_t *, void *);
	sigset13_t sa_mask;
	int sa_flags;
};

void darwin_sendsig(int, sigset_t *, u_long);

#endif /* _DARWIN_DARWIN_H_ */


/*
 * Copyright (c) 2010 The DragonFly Project. All rights reserved.
 * All rights reserved.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Venkatesh Srinivas <me@endeavour.zapto.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/asmacros.h>
#include <machine/cputypes.h>
#include <machine/pmap.h>
#include <machine/specialreg.h>

#include "assym.s"

	.text

/*
 * void cpu_mmw_pause_int(int *addr, int oldval, int cstate, int intrbrk)
 */	
ENTRY(cpu_mmw_pause_int)
	movq	%rdi, %rax
	movq	%rdx, %r8
	movq	%rcx, %r9

	xorq	%rcx, %rcx
	xorq	%rdx, %rdx
	monitor
	cmpl	(%rax), %esi
	jne	1f
	movq	%r8, %rax
	movq	%r9, %rcx
	mwait
1:
	ret

/*
 * void cpu_mmw_pause_long(long *addr, long oldval, int cstate, int intrbrk)
 */	
ENTRY(cpu_mmw_pause_long)
	movq	%rdi, %rax
	movq	%rdx, %r8
	movq	%rcx, %r9

	xorq	%rcx, %rcx
	xorq	%rdx, %rdx
	monitor
	cmpq	(%rax), %rsi
	jne	1f
	movq	%r8, %rax
	movq	%r9, %rcx
	mwait
1:
	ret

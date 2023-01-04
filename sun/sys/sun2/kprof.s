	.data
	.asciz	"@(#)kprof.s 1.1 86/09/25 Copyr 1985 Sun Micro"
	.even
	.text

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* 
 * Kernel profiling -- done at level 7 so nearly all code gets profiled.
 */
#include "assym.s"
#include "../machine/psl.h"

	.data
	.globl	_kprimin4, _kpri, _krate, _kticks, kprofinit, kprof
_kprimin4:	.word 0*4	| Lowest prio to trace PC of, times 4.
_kpri:		.long 0, 0, 0, 0, 0, 0, 0, 0
_krate:		.word (800 / 50)| (PI_RATE / hz), used in model 100s
_kticks:	.word 0
	.text

kprofinit:
				| no state to initialize
	rts

/*
 * The profiling routine.  This is entered on NMI (level 7 non-maskable
 * interrupt).  We quickly profile things then jump to the real NMI routine,
 * which performs memory refresh among other things.
 */
kprof:
	movl	d0,sp@-
	movl	a0,sp@-
	movw	sp@(8+4),d0	| SR stacked by interrupt
	andw	#SR_INTPRI,d0	| Current priority
	lsrw	#6,d0		| ...as 0-7, times size of bucket (4)
	lea	_kpri,a0
	addql	#1,a0@(0,d0:w)	| Bump priority bucket at A0 + D0
| Divisor stuff is for Model 100s where the clock interrupts
| 800 times a second.  On Model 120s it's 50 times a second.
| So, decomment these lines to profile on a 100.
|	subqw	#1,_kticks	| decrement "divisor"
|	bgt	kdone		| not time yet
|	movw	_krate,_kticks	| reset "divisor"
	cmpw	_kprimin4,d0
	jlt	kdone
	btst	#SR_SMODE_BIT-8,sp@(8+4) | were we in supervisor mode?
	jeq	kdone		| no, all done
	movl	sp@(10+4),d0	| PC stacked by interrupt
	cmpl	#2,_profiling
	jge	kdone
	movl	_kcount,a0
	subl	_s_lowpc,d0
	jlt	kdone
	cmpl	_s_textsize,d0
|	jge	kover
	jge	kdone
	lsrl	#2,d0
	lsll	#1,d0
	addl	d0,a0
kover:
	addqw	#1,a0@		| Bump some bucket corresp. to PC
kdone:
	movl	sp@+,a0		| Restore regs
	movl	sp@+,d0
	rts

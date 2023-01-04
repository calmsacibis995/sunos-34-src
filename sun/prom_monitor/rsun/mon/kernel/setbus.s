| 	@(#)setbus.s 1.5 83/09/19 Copyright (c) 1983 by Sun Microsystems, Inc.
|
|	setbus.s
|
|	Setjmp-style recovery from bus errors.
|
|	In C, code:
|		auto long busbuf[4];	
|		if (setbus(busbuf)) buserror; else normal;
|
|	This runs the "normal" code, but sneaks back to the "buserror" code
|	if a bus error occurs.
|
|	If the trap springs, it disables itself.
|	To disable it manually, call unsetbus(busbuf);
|

#include "assym.h"

	.text
	.globl	_setbus, _unsetbus
_setbus:
	movl	sp@(4),a0	| Get bus buffer area
	movl	sp@,a0@		| Save return address of setbus().
	movl	sp,a0@(4)	| Save stack pointer value
	movl	EVEC_BUSERR,a0@(8) 	| Save old bus error vector.
	movl	g_busbuf,a0@(12)	| Save old busbuf address
	movl	a0,g_busbuf	| Install new busbuf address
	movl	#buserr,EVEC_BUSERR	| Set up bus error vector
	moveq	#0,d0		| Result is zero
	rts

| Entered when a bus error occurs
buserr:
	movl	g_busbuf,a0	| Get bus buffer pointer
	movl	a0@(4),sp	| Restore stack pointer
	movl	a0@,sp@		| Restore return PC of setbus()
	moveq	#1,d0		| Result is 1, indicating error occurred.
	jra	unsetout	| Unset it and get out
_unsetbus:
	movl	sp@(4),a0	| Get bus buffer area
unsetout:
	movl	a0@(8),EVEC_BUSERR	| Restore old bus error vector
	movl	a0@(12),g_busbuf	| Restore old busbuf address
	rts


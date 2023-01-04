        .data 
        .asciz  "@(#)elems.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .text

#include "DEFS.h"
	.globl _sqrt
_sqrt:
	movl	sp@(4),d0
	movl	sp@(8),d1
	jmp	fvsqrti
	.globl	Fexpi,Sexpi
Fexpi:
Sexpi:
	movl	d1,sp@-
	movl	d0,sp@-
	jsr	_exp
	addql	#8,sp
	rts
	.globl	Flogi,Slogi
Flogi:
Slogi:
	movl	d1,sp@-
	movl	d0,sp@-
	jsr	_log
	addql	#8,sp
	rts
	.globl	Fsini,Ssini
Fsini:
Ssini:
	movl	d1,sp@-
	movl	d0,sp@-
	jsr	_sin
	addql	#8,sp
	rts
	.globl	Fcosi,Scosi
Fcosi:
Scosi:
	movl	d1,sp@-
	movl	d0,sp@-
	jsr	_cos
	addql	#8,sp
	rts
	.globl	Ftani,Stani
Ftani:
Stani:
	movl	d1,sp@-
	movl	d0,sp@-
	jsr	_tan
	addql	#8,sp
	rts
	.globl	Fatani,Satani
Fatani:
Satani:
	movl	d1,sp@-
	movl	d0,sp@-
	jsr	_atan
	addql	#8,sp
	rts
	.globl	Fpowi,Spowi
Fpowi:
Spowi:
	movl	d1,sp@-
	movl	d0,sp@-
	jsr	_pow
	addl	#8,sp
	rts
	.globl	Fpowis
Fpowis:
	movl	d0,a0		| Save x argument.
	movl	d1,d0		| d0 gets y argument.
	jsr	fvdoublei	| d0/d1 gets double(y).
	moveml	#0xc000,sp@-	| Push double(y) on stack.
	movl	a0,d0		| d0 gets x.
	jsr	fvdoublei	| d0/d1 gets double(x).
	moveml	#0xc000,sp@-	| Push double(x).
	jsr	_s_pow		| d0/d1 gets pow(x,y).
	addl	#16,sp		| Remove arguments.
	jsr	fvsinglei	| d0 gets single result.
	rts


        .data
|        .asciz  "@(#)Vstatus.s 1.1 86/09/24 Copyr 1986 Sun Micro"
        .even
        .text

|       Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

VECTORED(status)

	.text
ENTER(_fpstatus_)
	movel	sp@(4),a0	| Get address of long argument.
	movel	a0@,d0		| Get long argument.
	jmp	Vstatus

ENTER(getquotient)		| Get remainder quotient.
	jsr	Vstatus
	movel	d0,sp@-
	jsr	Vstatus
	movel	sp@+,d0
	swap	d0
	andw	#0x7f,d0
	btst	#23,d0
	beqs	1f
	negw	d0
1:
	extl	d0		| Old quotient.
	rts

ENTER(setquotient)		| Set remainder quotient.
	tstl	d0
	bpls	1f
	negb	d0
	bset	#7,d0
	bras	2f
1:
	bclr	#7,d0
2:
	andl	#0xff,d0	| Clear other bits.
	swap	d0
	movel	d0,sp@-
	jsr	Vstatus
	andl	#0xff00ffff,d0	| Clear quotient.
	orl	sp@+,d0		| Insert new quotient.
	jsr	Vstatus
	rts

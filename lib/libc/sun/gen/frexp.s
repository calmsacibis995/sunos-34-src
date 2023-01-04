        .data
|       .asciz  "@(#)frexp.s 1.1 86/09/24 Copyr 1986 Sun Micro"
        .even
        .text
 
|       Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

	NSAVED	 = 5*4	| save registers d2-d7
	SAVEMASK = 0x3f00
	RESTMASK = 0x00fc

	M0	= d0
	M1	= d1
	EXP	= d2
	TYPE	= d3
	/* type values: */
	    ZERO  = 1 | wonderful
	    GU    = 2
	    PLAIN = 3
	    INF   = 4
	    NAN   = 5

/*
 * double
 * frexp( value, eptr)
 *      double value;
 *      int *eptr;
 *
 * return a value v s.t. fabs(v) < 1.0
 * and return *eptr value e s.t.
 *       v * (2**e) == value
 */

ENTRY(frexp)
	movl	PARAM,d0	| suck parameters into registers
	movl	PARAM2,d1
	movl	PARAM3,a0
	moveml	#SAVEMASK,sp@-	| state save
	jbsr	d_unpk
	cmpb	#PLAIN,TYPE	| is it a funny number?
	beqs	2$		| Branch if normal.
	bgts	gohome		| yes -- repack and forget
	cmpb	#ZERO,TYPE
	bnes	1$
	clrl	a0@
	bras	gohome		| can't do much with zero, either
1$:
	subqw	#1,EXP		| Subnormal requires normalization.
	lsll	#1,d1
	roxll	#1,d0
	btst	#20,d0
	beqs	1$		| Branch if not yet normalized.
2$: 			| normal path through here
	addw	#(52+1),EXP	| return current exp val +1
				| 52 is unpacked bias -- mantissa has point on right?
	extl	EXP
	movl	EXP,a0@
	movw	#-53,EXP	| repack with exp val of -1
gohome:
	jbsr	d_pack
gone:	moveml	sp@+,#RESTMASK
	RET

        .data
        .asciz  "@(#)Satan2s.s 1.1 86/09/25 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

/*
	atan2(y,x) = atan(y/x)

	y is in d0
	x is in d1

*/

RTENTRY(Satan2s)
	moveml	d2/d3/d4/d5,sp@- | Save registers.
	movel	d1,d3		| d3 saves x.
	smi	d4		| d4 gets true if x < 0.
	movel	d0,d2		| d2 saves y.
	jsr	Sdivs		| d0 gets y/x.
	movel	d0,d5		| d5 gets quotient.
	bclr	#31,d5		| Abs quotient.
	cmpl	#0x7f800000,d5
	bles	3f		| Branch if |quotient| <= inf.  Can't be snan after divide.
	movel	d3,d1		| Restore x.
	movel	d2,d0		| Restore y.
	jsr	Fcmps
	bvss	nan
	movel	d2,d0
	eorl	d3,d0		| d0 gets proper sign of quotient.
	andl	#0x80000000,d0	| d0 gets isolated quotient sign.
	orl	#0x3f800000,d0	| d0 gets 1.0 with proper sign.
3:
	jsr	Satans		| d0 gets atan(y/x).
	tstb	d4	
	beqs	Satan2dend	| Branch if x >= 0.
	tstl	d2		| Test sign of y.
	bmis	1f		| Branch if y < 0.
	movel	pi,d1
	jsr	Sadds		| d0/d1 gets atan(y/x)+pi.
	bras	2f
1:	
	movel	pi,d1
	jsr	Ssubs		| d0/d1 gets atan(y/x)-pi.
2:	
Satan2dend:
	moveml	sp@+,d2/d3/d4/d5 | Restore registers.
	RET
nan:
	movel   d3,d1
        movel   d2,d0           | Restore y.
        jsr	Sadds		| Add will get a proper quiet nan result.	
	bras	Satan2dend

pi:	.single	0r3.1415926535897932384626433 

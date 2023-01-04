        .data
|       .asciz  "@(#)Fatan2d.s 1.1 86/09/25 Copyr 1986 Sun Micro"
        .even
        .text

|       Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

/*
	atan2(y,x) = atan(y/x)

	y is in d0/d1
	x is in a0@

*/

RTENTRY(Fatan2d)
	moveml	d2/d3/d4/d5/a0,sp@- | Save registers.
	tstb	a0@
	smi	d4		| d4 gets true if x < 0.
	movel	d0,d2		| d2/d3 save y.
	movel	d1,d3		| Save rest of y.
	jsr	Fdivd		| d0/d1 gets y/x.
	movel	d0,d5		| d5 gets top of quotient.
	bclr	#31,d5		| Abs quotient.
	cmpl	#0x7ff00000,d5
	bles	3f		| Branch if |quotient| <= inf.  Can't be snan after divide.
	movel	d3,d1
	movel	d2,d0		| Restore y.
	movel	sp@(16),a0	| Restore a0.
	jsr	Fcmpd
	bvss	nan
	movel	a0@,d0
	eorl	d2,d0		| d0 gets proper sign of quotient.
	andl	#0x80000000,d0	| d0 gets isolated quotient sign.
	orl	#0x3ff00000,d0	| d0/d1 gets 1.0 with proper sign.
	clrl	d1
3:
	jsr	Fatand		| d0/d1 gets atan(y/x).
	tstb	d4	
	beqs	Fatan2dend	| Branch if x >= 0.
	lea	pi,a0		| lea doesn't affect condition codes.
	tstl	d2		| Test sign of y.
	bmis	1f		| Branch if y < 0.
	jsr	Faddd		| d0/d1 gets atan(y/x)+pi.
	bras	2f
1:	
	jsr	Fsubd		| d0/d1 gets atan(y/x)-pi.
2:	
Fatan2dend:
	moveml	sp@+,d2/d3/d4/d5/a0 | Restore registers.
	RET
nan:
	movel	sp@(16),a0	| Restore x.
	movel   d3,d1
        movel   d2,d0           | Restore y.
        jsr	Faddd		| Add will get a proper quiet nan result.	
	bras	Fatan2dend

pi:	.double	0r3.1415926535897932384626433 

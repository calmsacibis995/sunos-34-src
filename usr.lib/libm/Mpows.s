        .data
        .asciz  "@(#)Mpows.s 1.1 86/09/25 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(Mpows)
        fabss	d0,fp0		| fp0 gets |x|.
	flognx	fp0,fp0
	fmuls	d1,fp0		| fp0 gets y * log(|x|).
	fetoxx	fp0,fp0		| fp0 gets |x|**y.
        tstl	d0
	bpls	1f		| Branch if x > 0.
	movel	d1,sp@-		| Save y.
	bclr	#31,d1		| d1 gets |y|.
	cmpl	two24,d1
	bges	3f		| Branch if y was large (even) integer.
	movel	d0,sp@-		| Save x.
	fmoves	d1,fp1		| fp1 gets |y|.
	fmovel	fp1,d1		| d1 gets int(|y|).
	fmovel	fps,d0		| d0 gets status.
	andw	#0xff00,d0	| d0 gets current exceptions.
	bnes	bad		| Branch if y was not integer.
	addql	#8,sp		| Restore sp.
 	btst	#0,d1
	beqs	1f		| Branch if y was even integer.
4:
	fmoves	fp0,d0
	bchg	#31,d0		| Reverse sign of result.
	bras	2f
3:
	addql	#4,sp		| Restore sp.
	bras	1f
bad:
	moveml	sp@+,d0/d1	| Restore x, y.
	flogns	d0,fp0
	fmuls	d1,fp0
	fetoxx	fp0,fp0
	bras	4b		| Negate result to cover case x = -0.
1:
	fmoves	fp0,d0
2:
	RET

two24:	.single	0r1.6777216e+7	| Threshold of even integral values.

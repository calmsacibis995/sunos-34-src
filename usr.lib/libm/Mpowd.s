        .data
|       .asciz  "@(#)Mpowd.s 1.1 86/09/25 Copyr 1986 Sun Micro"
        .even
        .text

|       Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(Mpowd)
	moveml	d0/d1/d2,sp@-	| Save x  and fpcr on stack.
	fmovel	fpcr,d2		| d2 gets original fpcr.
	movel	d2,a1		| a1 gets original fpcr.
	clrb	d2		| d2 gets default rounding dir & prec.
	fmovel	d2,fpcr		| fpcr gets default rounding.
	fabsd	sp@,fp0		| fp0 gets |x|.
        fcmps   #0r0.5,fp0
        fjole   complex         | Branch if |x| <= 0.5.
        fcmpw	#2,fp0
	fjult	simple		| Branch if |x| < 2.
complex:
        fgetexpx fp0,fp1	| fp1 gets the exponent of x.
	fnegx	fp1,fp1		| fp1 gets -exp(x).
	fscalex	fp1,fp0		| fp0 gets scaled to near 1.
	fsubl   #1,fp0
	flognp1x fp0,fp0        | This is more accurate for x near 1.
	fmuld	a0@,fp0
	fetoxx 	fp0,fp0		| fp0 gets |x|**y.
	fgetexpd sp@,fp1	| fp1 gets exponent of x again.
	fmuld	a0@,fp1		| fp1 gets exponent * y.
	ftwotoxx fp1,fp1	| fp1 gets 2** (y * exponent).
	fmulx	fp1,fp0		| Combine.
	fjor	xneg		| Branch if result is not NaN.
	fabsd	sp@,fp0		| Restore |x|.
simple:
	fsubl   #1,fp0
        flognp1x fp0,fp0        | This is more accurate for x near 1.
	fmuld	a0@,fp0		| fp0 gets y * log(|x|).
	fetoxx 	fp0,fp0		| fp0 gets |x|**y.
xneg:
        tstl	d0
	bpls	1f		| Branch if x > 0.
	movel	a0@(4),sp@-
	movel	a0@,sp@-	| Push y.
	jsr	_d_integral	| Classifies y as even, odd, or non-integral.
	addql	#8,sp		| Remove arguments from stack.
	cmpw	#1,d0
	bgts	1f		| Branch if y was large (even) integer.
	blts	bad		| Branch if y was not integer.
3:
	fnegx	fp0,fp0		| Reverse sign of result.
	bras	1f
bad:
	tstl	a0@(4)
	bnes	5f		| Branch if y not +-0.
	movel	a0@,d0
	bclr	#31,d0
	tstl	d0
	beqs	1f		| Branch if y is +-0 - really even.
5:
	flognd	sp@,fp0
	fmuld	a0@,fp0		| fp0 gets y * log(x).
	fetoxx	fp0,fp0		| fp0 gets x**y.
	bras	3b		| Negate result to cover case x = -0.
1:
	movel	a1,d2		| d2 gets original fpcr.
	fmovel	d2,fpcr		| Restore fpcr for final store.
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1/d2	| Install result and restore d2.
	RET

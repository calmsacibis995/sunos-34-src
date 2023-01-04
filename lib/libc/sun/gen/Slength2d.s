	.data
|	.asciz	"@(#)Slength2d.s 1.1 86/09/24 Copyr 1986 Sun Micro"
	.even
	.text


|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Sdefs.h"

RTENTRY(Slength2d)
	link	a6,#-12
scalefactor = -4			| temp on stack
ysquared = -12				| temp on stack
	moveml	d0/d1,sp@-		| Save d0/d1/d2/d3/d4.
	jsr	Fexpod			| d0 gets exponent of x.
	movel	d0,d1			| d1 gets exponent of x.
	movel	a0@,d0			| d0 gets upper part of y.
	jsr	Fexpod			| d0 gets exponent of y.
	cmpl	d0,d1
	bges	1f
	movel	d0,a6@(scalefactor)	| gets scale factor.
	bras	2f
1:
	movel	d1,a6@(scalefactor)	| gets scale factor.
2:
	negl	a6@(scalefactor)	| Reverse sign.
	moveml	a0@,d0/d1		| d0/d1 get y.
	lea	a6@(scalefactor),a0	| a0 gets address of scale factor.
	jsr	Fscaleid		| d0/d1 get y/s.
        movl    __skybase,SKYBASE
        FMULD(d0,d1,d0,d1,a6@(ysquared),a6@(ysquared+4)) | ysquared get (y/s)**2
	moveml	sp@+,d0/d1		| Restore x.
	lea	a6@(scalefactor),a0	| a0 gets address of scale factor.
	jsr	Fscaleid		| d0/d1 get x/s.
	movw	#0x1018,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movel	a6@(ysquared),SKYBASE@
	movel	a6@(ysquared+4),SKYBASE@
	IORDY				| r0 := y * y
	movl	SKYBASE@,d0
	movl	SKYBASE@,d1		| d0/d1 := x * x + y * y
	jsr	Fsqrtd
	negl	a6@(scalefactor)	| Reverse scale factor again.
	lea	a6@(scalefactor),a0	| a0 gets address of scale factor.
	jsr	Fscaleid
	unlk	a6
	RET

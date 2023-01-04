        .data
        .asciz  "@(#)Fclass1d.s 1.1 86/09/24 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include <sys/errno.h>

|	Fclass1d is a front end routine for floating point software.
|	It is called as follows:
|	
|	Fxxxd:	jsr	Fclass2d
|		.long	xnorm,xzero,xsub,xinf
|
|	Each table entry is a small odd negative integer or an address.  The integers correspond
|	to predefined routines for common operations.  The addresses are addresses of
|	caller-supplied routines.  x is classified, and if the indicated table entry
| 	is one of the predefined routines, that routine is executed followed by return to
|	the caller of Fxxxd.  Otherwise the entry point listed in the routine is jumped to with
|	a stack frame containing:
|
|	top:	original d0-d3/a1 save
|		Fxxxd+6, the jsr return
|
|	d0/d1/d3 contain unpacked x.
|	The sign of x is in bit 31 of d3, the unbiased exponent in bits 0 to 15,
|	the significand in d0 and d1, normalized.
|	Note that predefined routines should be used if either argument is zero or infinity.
|	NaNs are checked for automatically and return quiet NaNs.

offnorm	= 0
offzero = 1
offsub  = 2
offinf  = 3

ENTER(Fclass1d)
	moveml	d0-d3/a1,sp@-		| Save all d registers.
	movel	sp@(20),a1		| a1 gets address of jump table.
	roll	#4,d0
	roll	#8,d0			| Align exponent.
	movel	d0,d3
	andl	#0x7ff,d3		| d3 gets biased exponent.
	beqs	xmin			| Branch if x is 0 or subnormal.
	cmpw	#0x7ff,d3
	beqs	xmax			| Branch if x is inf or nan.
	addl	#4*offnorm,a1		| a1 gets offset for x.
	bset	#0,d0			| Turn on i bit.
	bras	fetchtable
xmax:	
	eorl	d0,d3			| Turns on s and f bits in d0.
	bclr	#11,d3			| Turn off sign bit.
	orl	d1,d3			| Throw in rest of significand.
	bnes	xnan
	addl	#4*offinf,a1		| a1 gets offset for x.
	bras	fetchtable
xnan:
	bset	#19,sp@(1)		| Make x quiet.
	jra	returnx
xmin:	
	movel	d0,d2
	bclr	#11,d2			| Clear sign bit.
	orl	d1,d2
	bnes	xsub
	addl	#4*offzero,a1		| a1 gets offset for x.
	bras	fetchtable
xsub:
	addl	#4*offsub,a1		| a1 gets offset for x.
	bclr	#0,d0			| Turn off i bit.
	movew	#1,d3			| Subnormal exponent.
	bras	fetchtable
fetchtable:
	movel	a1@,a1			| a1 gets table entry.
	cmpl	#0,a1
	bgts	unpack
	cmpl	#RETURNX,a1
	beqs	returnx
	cmpl	#RETURNINVALID,a1
	beqs	returninvalid	
	cmpl	#RETURN1,a1
	beqs	return1
	cmpl	#RETURNINFORZERO,a1
	beqs	returninforzero
	bras	returninvalid

returninforzero:
	tstb	sp@
	bmis	1f
	moveml	inf,d0/d1
	bras	bypassx			| +: return +inf.
1:
	moveml	zero,d0/d1
	bras	bypassx
return1:
	moveml	one,d0/d1
	bras	bypassx
returninvalid:
	movel   #EDOM,_errno
	moveml	invalidnan,d0/d1
bypassx:
	addql	#8,sp			| Bypass x.
	moveml	sp@+,d2-d3/a1		| Restore registers.
	bras	bypass
returnx:
	moveml	sp@+,d0-d3/a1		| Restore x and d registers.
bypass:
	addql	#4,sp			| Bypass return address of Fclass2d.
	rts

zero:
	.double	0r0.0
one:
	.double 0r1.0
inf:
	.double 0rInf
invalidnan:
	.double	0rNaN
	
|	Unpack x.

unpack:
	subw	#0x3ff,d3		| Unbias x exponent.
	btst	#11,d0
	beqs	1f			| Branch if x positive.
	bset	#31,d3			| Negative sign in d3.
1:
	rorl	#1,d0			| Align significand.
	andl	#0xfffff800,d0		| Remove sign and exponent from d0.
	roll	#8,d1
	roll	#3,d1
	movel	d1,d2
	andl	#0x7ff,d2		| d2 gets low order bits.
	orl	d2,d0
	andl	#0xfffff800,d1
	tstl	d0
	bmis	1f
2:
	subqw	#1,d3
	lsll	#1,d1
	roxll	#1,d0
	bpls	2b
1:	
	jmp	a1@			| Not predefined, so go to it.

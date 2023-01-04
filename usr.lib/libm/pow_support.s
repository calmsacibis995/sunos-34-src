        .data 
        .asciz  "@(#)pow_support.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .text

#include "fpcrtdefs.h"

| int d_integral(x)
| double x ;
|	returns 0 for not integral value; 1 for odd integral value;
|		2 for even integral value
|	only defined for normalized numbers.

RTENTRY(_d_integral)
	movl	PARAM,d0
	movl	PARAM2,d1
	moveml	#0x3000,sp@-	| Save d2/d3.
	movl	d0,d2		| d2 gets exponent etc.
	swap	d2		| lower(d2) gets exponent etc.
	bclr	#15,d2		| Clear sign.
	lsrw	#4,d2		| Right justify biased exponent.
	subw	#0x3ff,d2	| Subtract bias.
	bmi	no		| Exp <= -1 means not integral.
	cmpw	#52,d2
	bges	ge52		| Exp >= 52 means integral.
			| 0 <= exp <= 51.
	cmpw	#20,d2
	bgts	gt20
	blts	lt20
				| Exp = 20.
	tstl	d1
	bnes	no		| Branch if not integral.
	btst	#0,d0
	bras	testodd
gt20:			| 21 <= exp <= 51.
	negw	d2
	addw	#52,d2		| d2 gets 1 <= 52-exp <= 29.
	clrl	d3
	bset	d2,d3		| Set bit representing 1.0.
	movl	d3,d2		| d2, d3 get 1 bit.
	subql	#1,d2		| d2 gets mask of fraction bits.
	andl	d1,d2
	bnes	no		| Branch if not integral.
	andl	d1,d3
	bras	testodd
lt20:			| 0 <= exp <= 19.
	tstl	d1
	bnes	no		| Branch if not integral because of lower word.
	negw	d2
	addw	#20,d2		| d2 gets 1 <= 20-exp <= 20.
	clrl    d3
        bset    d2,d3           | Set bit representing 1.0.
        movl    d3,d2           | d2, d3 get 1 bit.
        subql   #1,d2           | d2 gets mask of fraction bits.
        andl    d0,d2
        bnes    no              | Branch if not integral.
        bset	#20,d0		| Implicit I bit is always on!
	andl    d0,d3
        bras    testodd
ge52:
	bgts	even		| Exp >= 53 means even integral.
	btst	#0,d1		| Exp = 52.
testodd:
	beqs	even
odd:
	movl	#1,d0
	bras	iret
even:
	movl	#2,d0
	bras	iret
no:
	clrl	d0
iret:	
	moveml	sp@+,#0x0c	| Restore d2/d3.
	RET

| int d_powexp( double d )
| for subnormal or normal d, returns the exponent such that
| ldexp( d, -exp) would be between sqrt(0.5) and sqrt(2.0) in magnitude.
| Thus log2(ldexp) would be between +- 0.5.

RTENTRY(_d_powexp)
	movl	PARAM,d0
	movl	PARAM2,d1
	movl	d2,sp@-		| Save d2.
	roll	#8,d0
	roll	#4,d0
	movl	d0,d2		| d2 gets significand, d0 gets exponent.
	andl	#0x7ff,d0	| Clear junk.
	beqs	subnormal	| Branch if subnormal.
	subl	#0x3ff,d0	| Remove bias.
	andw	#0xf000,d2	| Clear junk.
	cmpl	#0x6a09f000,d2	| 
				| Leading part of sqrt(2).
	bcss	ret
	bhis	adjust		| Branch if significand > sqrt(2).
	cmpl	#0x667f3bcc,d1	| Lower part of sqrt(2).
	blss	ret
adjust:
	addql	#1,d0		| Adjust exponent.
ret:
	movl	sp@+,d2		| Restore d2.
	RET
subnormal:
	roll	#8,d1
	roll	#4,d1
	movw	d1,d0
	andw	#0xfff,d0	| Get 12 most significant bits of d1.
	orw	d0,d2		| Move to d2.
	andl	#0xfffff000,d1	| Clear bits.
	movl	#-1022,d0	| Set up minimal exponent.
shift:
	subql	#1,d0
	lsll	#1,d1
	roxll	#1,d2
	bccs	shift		| Branch if bit not found yet.
	cmpl	#0x6a09f667,d2	| Upper part of sqrt(2).
	bcss	ret
	bhis	adjust
	cmpl	#0xf3bcc908,d1
	bcss	ret
	bras	adjust		| Can't be equal!

| 	double d_intfrac( double d, *double f )
|
|	Given d, which must be a normal number, returns its integer and
|	fraction part so that d = int + frac.  The magnitude of the fraction
|	is between 0 and 1.

RTENTRY(_d_intfrac)
	movl	PARAM,d0
	movl	PARAM2,d1
	movl	PARAM3,a0	| Address for fraction.
	moveml	#0x3c00,sp@-	| Save d2/d5.
	movl	d0,d4		| d4 gets exponent etc.
	swap	d4		| lower(d4) gets exponent etc.
	bclr	#15,d4		| Clear sign.
	lsrw	#4,d4		| Right justify biased exponent.
	subw	#0x3ff,d4	| Subtract bias.
	bmi	intzero		| Exp <= -1 means int=0, frac=d.
	cmpw	#52,d4
	bges	fraczero	| Exp >= 52 means int=d, frac=0.
			| 0 <= exp <= 51.
	movl	d0,d2		| Frac = int = x.
	movl	d1,d3
	cmpw	#20,d4
	bgts	ifgt20
	blts	iflt20
				| Exp = 20.
	clrl	d1		| Clear fraction bits from int.
	clrl	d2		| Clear int bits from frac.
	bras	normfraction
ifgt20:			| 21 <= exp <= 51.
	negw	d4
	addw	#52,d4		| d4 gets 1 <= 52-exp <= 29.
	clrl	d5
	bset	d4,d5		| Set bit representing 1.0.
	subql	#1,d5		| d5 gets mask of fraction bits.
	clrl	d2
	andl	d5,d3		| Clear int bits from frac.
	notl	d5
	andl	d5,d1		| Clear frac bits from int.
	bras	normfraction
iflt20:			| 0 <= exp <= 19.
	negw	d4
	addw	#20,d4		| d4 gets 1 <= 20-exp <= 20.
	clrl    d5
        bset    d4,d5           | Set bit representing 1.0.
        subql   #1,d5           | d5 gets mask of fraction bits.
        andl	d5,d2		| Clear int bits from frac.
	notl	d5
	andl	d5,d0		| Clear frac bits from int.
	clrl	d1
normfraction:			| Fraction in d2 and d3 requires normalization.
	movl	d0,d4		| d4 gets sign/exponent.
	swap	d4
	tstl	d2
	bnes	donorm
	tstl	d3
	beqs	fraczero	| Branch if fraction is exact zero.
shift16:
	swap	d3		| Do 16 bit left shift.
	movw	d3,d2
	clrw	d3
	subw	#0x100,d4	| Subtract 16 from exponent.
	tstl	d2
	beqs	shift16		| Branch if another 16 bit shift required.
normloop:			| Normalize fraction.
	subw	#0x010,d4	| Subtract 1 from exponent.
	lsll	#1,d3
	roxll	#1,d2
donorm:
	btst	#20,d2
	beqs	normloop
	andl	#0xfff0,d4	| Clear other stuff.
	swap	d4		| Now sign and exponent are properly aligned.
	bclr	#20,d2		| Clear I bit.
	orl	d4,d2		| Insert sign and exponent.
	bras	ifret		
fraczero:			| Big number so fraction = 0, int = x.
	movl	d0,d2
	andl	#0x80000000,d2	| Frac = signed zero.
	clrl	d3
	bras	ifret	
intzero:
	movl	d0,d2		| Small number so fraction = x, int = 0.
	movl	d1,d3		| Fraction part = x.
	andl	#0x80000000,d0	| Int = signed zero.
	clrl	d1
ifret:	
	moveml	#0xc,a0@	| Store fraction part from d2/d3.
	moveml	sp@+,#0x3c	| Restore d2/d5.
	RET


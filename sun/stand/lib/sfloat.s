	.data
	.asciz "@(#)sfloat.s 1.1 9/25/86 Copyright Sun Micro"
	.even
	.text

|	Copyright (c) 1983 by Sun Microsystems, Inc.


/*
 *	ieee single floating run-time support
 *	copyright 1981, Richard E. James III
 *	translated to SUN idiom 14 March 1983 rt
 *	make part of SA library kss Tue Jan 17 16:01:12 PST 1984
 */
#include "DEFS.h"

/*	single floating addition and subtraction	*/
/*
 *	entry conditions:
 *	    first argument in d0
 *	    second argument in d1
 *	exit conditions:
 *	    result (4 bytes) in d0
 *
 *	register conventions:
 *	    d0		operand 1
 *	    d1		operand 2/sum
 *	    d2		sign of operand 1
 *	    d3		sign of operand 2
 *	    d4		exponent of operand 1
 *	    d5		exponent of operand 2
 *	    d6		difference of exponents
 *	    d7		reserved for sticky
 */
	SAVEMASK = 0x3f00	| registers d2-d7
	RESTMASK = 0xfc
	NSAVED   = 6*4		| 6 registers * sizeof(register)

RTENTRY(Fsubis)
	bchg	#31,d1
	jra	adding
RTENTRY(Faddis)
adding:
|	save registers and load operands into registers
	moveml	#SAVEMASK,sp@-	| registers d2-d7
	| extract signs
	asll	#1,d0	| sign ->c
	scs	d2	| c -> d4
	asll	#1,d1
	scs	d3
	| compare and exchange to put larger in d0
	cmpl	d1,d0
	blss	1$
	exg	d0,d1
	exg	d2,d3
	| extract exponents
1$:	roll	#8,d1
	roll	#8,d0
	clrl	d5
	movb	d1,d5		| larger exp
	clrw	d4
	movb	d0,d4		| smaller exp
	bnes	3$		| not zero or denormalized
	| smaller is 0 or is denormalized
	tstl	d0		| if smaller == 0
	beqs	usel		| then use larger (sign of 0-0 unpredictable)
	tstb	d5		| larger exponent
	bnes	4$		| not gu
	| both are denormalized
	cmpb	d3,d2		| compare signs
	bnes	2$
	addl	d0,d1
	addxb	d1,d1		| incr exp to 1 if overflow
	bras	asbuild
2$:	subl	d0,d1		| subtract
	bras	asbuild
	| neither is denormalized
3$:	movb	#1,d0		| clear exp and
	rorl	#1,d0
4$:	movb	#1,d1		| ... clr hidden bit
	rorl	#1,d1
	cmpb	#0xff,d5
	beqs	ovfl		| inf or nan
	| align smaller
	movw	d5,d6
	subw	d4,d6		| difference of exponents
6$:	cmpw	#8,d6
	blts	7$		| exit when <8 *s
	subqw	#8,d6
	lsrl	#8,d0		| align
	bnes	6$		| loop
7$:	lsrl	d6,d0		| finish align
	cmpb	d3,d2		| cmp signs
	bnes	diff		| decide whether to add or subtract

	| add them
	addl	d0,d1		| sum
	bccs	endas		| no carry here
	roxrl	#1,d1		| pull in overflow *s
	addqw	#1,d5
	cmpw	#0xff,d5
	blts	endas		| no ofl
	bras	a_geninf

	| subtract them
diff:	subl	d0,d1
	bmis	endas		| if normalized
	beqs	cancel		| result == 0
9$:	asll	#1,d1		| normalize
	dbmi	d5,9$		| dec exponent
	subqw	#1,d5
	bgts	endas		| not gu
	beqs	10$
	clrw	d5
	lsrl	#1,d1		| grad und
10$:	lsrl	#1,d1

endas:	| round (NOT FULLY  STANDARD)
	addl	#0x80,d1	| round
	bccs	12$		| round did not cause mantissa to ofl
	roxrl	#1,d1
	addqw	#1,d5
	cmpw	#0xff,d5
	beqs	a_geninf		| round caused exp to overflow
	| rebuild answer
12$:	lsll	#1,d1		| toss hidden
usel:	movb	d5,d1		| insert exponent
asbuild:rorl	#8,d1
	roxrb	#1,d3
	roxrl	#1,d1		| apply sign
	| d1 now has answer
	movl	d1,d0
asexit:
	moveml	sp@+,#RESTMASK
	RET

	| EXCEPTION CASES
ovfl:	| largest exponent = 255
	lsll	#1,d1
	tstl	d1		| larger mantissa
	bnes	usel		| larger == nan
	cmpb	d4,d5		| exps
	bnes	usel		| larger == inf
	| AFFINE MODE ASSUMED IN THIS IMPLEMENTATION
	cmpb	d3,d2		| signs
	beqs	usel		| inf+inf = inf
|gennan:
	movl	#0x7f800001,d0
	bras	asexit

	| complete cancellation
cancel:	clrl	d0		| result == 0
	| need minus 0 for (-0)+(-0), round to -inf
	bras	asexit
	| result overflows:
a_geninf:movl	#0xff,d1
	bras	asbuild


/*
 *	ieee single floating compare
 *	copyright 1981, Richard E. James III
 *	translated to SUN idiom 29 March 1983 rt
 */

/*
 *	entry conditions:
 *	    first argument in d0
 *	    second argument in d1
 *	exit conditions:
 *	    result in condition code
 *	    d0/d1 trashed
 *
 *	register conventions:
 *	    d0		operand 1
 *	    d1		operand 2
 *	    d2		scratch
 */
	NSAVED	= 4
	CODE	= NSAVED

RTENTRY(Fcmpis)
	subqw	#2,sp		| save space for condition code return
	movl	d2,sp@-		| save register

	movl	d1,d2
	andl	d0,d2		| compare signs
	|bmis	nbothmi
	bpls	$1
	exg	d0,d1		| both minus
$1:	cmpl	d1,d0		| main compare
	andb	#0xE,cc		| clear carry
	movw	cc,sp@(CODE)
	lsll	#1,d0
	lsll	#1,d1
	cmpl	d1,d0
	bccs	2$
	exg	d0,d1	| find larger
2$:	cmpl	#0xff000000,d0
	blss	3$
	| nan
	movw	#1,sp@(CODE)	| c, nz
3$:	tstl	d0
	bnes	4$
	movw	#4,sp@(CODE)	| -0 == 0
	| result is in sp@(CODE)
4$:	| restore saved register and go
	movl	sp@+,d2
#ifdef PROF
	unlk	a6
#endif
	rtr

/*
 *	ieee single floating multiply
 *	copyright 1981, 1982 Richard E. James III
 *	translated to SUN idiom 14 March 1983 rt
 */

/*
 *	entry conditions:
 *	    first argument in d0
 *	    second argument in d1
 *	exit conditions:
 *	    result (4 bytes) in d0
 *
 *	register conventions:
 *	    d0		operand1/upper1
 *	    d1		operand2/upper2
 *	    d2		9/lower1
 *	    d3		lower2
 *	    d4		exponent
 *	    d5		sign
 */
	SAVEMASK = 0x3c00	| registers d2-d5
	RESTMASK = 0x3c
	NSAVED   = 6*4		| 6 registers * sizeof(register)

RTENTRY(Fmulis)
|	save registers
	moveml	#SAVEMASK,sp@-	| registers d2-d7
	| save sign of result
	movl	d0,d5
	eorl	d1,d5		| sign of result
	asll	#1,d0		| toss sign
	asll	#1,d1		| EEmmmm0
	cmpl	d1,d0
	| order operands (exponents at least)
	blss	eswap
	exg	d0,d1		| d1 = larger
	| extract and check exponents
eswap:	roll	#8,d0
	roll	#8,d1		| mmmmm0ee
	clrw	d4
	movb	d0,d4
	clrw	d3
	movb	d1,d3
	addw	d3,d4		| result exp
	cmpb	#0xff, d1
	beqs	ofl		| infinity or nan
	tstb	d0
	beqs	ufl		| 0 or gu (denormalized)
	| clear exponent; set hidden bit
	movb	#1,d0
	rorl	#1,d0
back:	movb	#1,d1
	rorl	#1,d1
	| split mantissas into 2 pieces
	movw	d0,d2		| lower
	movw	d1,d3
	swap	d0		| upper
	swap	d1
	| multiply the pieces
	mulu	d0,d3		| u1*l2
	mulu	d1,d2		| u2*l1
	mulu	d1,d0		| u2*u1
	| add together
	addl	d2,d3		| middle products
	addxb	d3,d3		| carry to bit 0
	andw	#1,d3		| toss rest
	swap	d3
	addl	d3,d0
	subw	#126,d4		| toss extra bias
	jbsr	f_rcp		| round check, pack

	| build answer
mbuild:	rorl	#8,d0
	roxll	#1,d5
	roxrl	#1,d0		| append sign

	| answer in d0
mexit:	moveml	sp@+,#RESTMASK
	RET

	| EXCEPTION HANDLING
ofl:	clrb	d1
	tstl	d1		| larger mantissa
	bnes	mni		| user larger nan
	tstl	d0
	beqs	m_gennan		| 0*inf
mni:	movb	#0xff,d1	| inf or nan
	movl	d1,d0
	bras	mbuild
ufl:	tstl	d0		| mantissa of smaller
	beqs	mbuild
	| normalizing mode is embodied int the next few lines:
	bmis	back
normden:subql	#1,d4		| adj exponent
	lsll	#1,d0
	bpls	normden
	bras	back

m_gennan:movl	#0x7f800002, d0
	bras	mexit

/*
 *	ieee single floating divide
 *	copyright 1981, Richard E. James III
 *	translated to SUN idiom 14 March 1983 rt
 */

/*
 *	entry conditions:
 *	    first argument in d0
 *	    second argument in d1
 *	exit conditions:
 *	    result (4 bytes) in d0
 *
 *	register conventions:
 *	    d0		top; ab;  rq
 *	    d1		bot; c
 *	    d2		     q
 *	    d3		bottom exp; d
 *	    d4		top/ final exp
 *	    d5		sign 
 */
|
|	same as for multiply, above
|	SAVEMASK = 0x3c00	| registers d2-d5
|	RESTMASK = 0x3c
|	NSAVED   = 4*4		| 4 registers * sizeof(register)

RTENTRY(Fdivis)
|	save registers
	moveml	#SAVEMASK,sp@-	| registers d2-d5
	| determine sign
	roll	#1,d0
	roll	#1,d1
	movb	d0,d5
	eorb	d1,d5		| sign in bit 0
	| split out exponent
	roll	#8,d0
	roll	#8,d1
	clrw	d3
	clrw	d4
	movb	d0,d4		| exp of top
	movb	d1,d3		| exp of bottom
	andw	#0xfe00,d0	| clear out s, exp
	andw	#0xfe00,d1	| clear out s, exp
	| test exponents
	addqb	#1,d4		| top
	subqw	#1,d4
	bles	toperr
	addqb	#1,d0		| hidden bit
backtop:addqb	#1,d3
	subqw	#1,d3		| bottom
	bles	boterr
	addqb	#1,d1		| hidden bit
	| position mantissas
backbot:rorl	#2,d1		| 01X...
	rorl	#4,d0		| 0001X...
	| compute tentative exponent
	subw	d3,d4
	| to compute ab/cd:
	|    first do ab/c -> q, remainder -> r
	movw	d1,d3		| save d
	swap	d1		| get c
	divu	d1,d0		| ab/c 29/15->15 bits
	movw	d0,d2		| save q
	mulu	d2,d3		| q*d
	clrw	d0		| r in top
	subl	d3,d0		| r-q*d = +-31
	asrl	#2,d0		| avoid ofl
	divs	d1,d0		| more quotient
	extl	d2		| q
	extl	d0		| second quot
	swap	d2
	asll	#2,d0
	addl	d2,d0
	asll	#1,d0
	| adjust exponent, round, check extremes, pack
	addw	#127,d4
	jbsr	f_rcp
	| reposition and append sign
drepk:	rorl	#8,d0
	roxrb	#1,d5		| sign -> x
	roxrl	#1,d0		| insert sign
dexit:	moveml	sp@+,#RESTMASK
	RET

	| EXCEPTIONS
toperr:	bnes	2$
	|top is 0 or gu, normalize and return
1$:	subqw	#1,d4
	roll	#1,d0
	bhis	1$		| loop til normalized, fall if 0
	addqw	#1,d4
	bras	backtop		| 0 or gu

	| top is inf or nan
2$:	cmpb	d3,d4
	beqs	dinvop		| both inf/nan -> nan
	tstl	d0
	beqs	geninf		| inf/ ... = +- inf
	bras	dinvop		| nan/...  =    nan

boterr:	beqs	botlow		| .../(0|gu)
				| .../(inf|nan)
	tstl	d1
	bnes	dinvop		| .../nan = nan
	clrl	d0		| .../inf = +-0
	bras	drepk
botlow:	tstl	d1
	beqs	5$		| .../0
				| .../gu:
4$:	subqw	#1,d3
	roll	#1,d1
	bccs	4$		| loop til normalized
	addqw	#1,d3
	bras	backbot

	| bottom is zero
5$:	tstl	d0
	beqs	d_gennan		| 0/0       =   nan
				| nonzero/0 = +-inf
	| generate infinity for answer
geninf:	movl	#0xff,d0
	bras	drepk

	| invalid operand/ operation
dinvop:	cmpl	d0,d1
	bcss	8$		| use larger nan
	tstl	d1
	beqs	d_gennan	| both are infinity, generate a nan
	exg	d1,d0		| larger nan
8$:	lsrl	#8,d0
	lsrl	#1,d0
	bras	bldnan		| return nan

d_gennan:moveq	#4,d0		| nan 4
bldnan:	orl	#0x7f800000,d0
	bras	dexit

	.globl	ieeeused


	.data
	.asciz	"@(#)float.s 1.1 86/09/24 Copyr 1983 Sun Micro"
	.even
	.text

|	Copyright (c) 1983 by Sun Microsystems, Inc.

/*
 *	double-precision floating math run-time support
 *	IEEE format & almost IEEE operation
 *	some single-precision utility operations included, too
 *
 *
 *	copyright 1981, 1982 Richard E. James III
 *	translated to SUN idiom 10/11 March 1983 rt
 *	parameter passing re-done 22 July 1983 rt
 */
#include "DEFS.h"

	NSAVED	 = 3*4	| save registers d2-d4
	SAVEMASK = 0x3c00
	RESTMASK = 0x003c
	ARG2PTR	 = a0

/*
 * type conversion operators
 *
 * integer-to-double conversion: fflti
 * input:
 *	d0	integer
 * output:
 *	d0/d1	double floating value
 */

RTENTRY(Fflti)
	moveml	#SAVEMASK,sp@-
	movl	d0,d1
	clrl	d0
	jbsr	i_unpk
	bras	xit_2_d

/*
 * single-to-double float conversion: fdoublei
 * input:
 *	d0	float
 * output:
 *	d0/d1	double floating value
 */

RTENTRY(Fdoublei)
	moveml	#SAVEMASK,sp@-
	movl	d0,d1
	jbsr	f_unpk
xit_2_d:bsr	d_pack
	moveml	sp@+,#RESTMASK
	RET

/*
 * double-to-integer conversion: ffixi
 * input:
 *	d0/d1	double floating value
 * output:
 *	d0	integer
 */

RTENTRY(Ffixi)
	moveml	#SAVEMASK,sp@-
	bsrs	d_unpk
	jbsr	i_pack
	bras	d_2_xit

/*
 * double-to-float conversion: fsinglei
 * input:
 *	d0/d1	double floating value
 * output:
 *	d0	integer
 */

RTENTRY(Fsinglei)
	moveml	#SAVEMASK,sp@-
	bsrs	d_unpk
	jbsr	f_pack
d_2_xit:
	movl	d1,d0	| oops -- pack always returns in d1!
	moveml	sp@+,#RESTMASK
	RET

/*
 * split a double floating number into its pieces
 * input:
 *	d0/d1	double number
 * output: (format of an unpacked record)
 *	d0/d1	mantissa: array[1..2] of integer;
 *	d2.w	exponent:	-32768..32767;
 *	d3.w	sign: (bit 15)  0..1;
 *	d3.b	type: 1..5; (zero, gu, plain, inf, nan )
 */

	.globl	d_unpk
d_unpk:
	movl	#0xfff00000,d2	| mask for sign and exponent
	movl	d0,d3
	swap	d3		| sign
	andl	d0,d2		| extract exponent
	eorl	d2,d0		| top of mantissa cleared out
	movl	d1,d4
	orl	d0,d4		| non-zero iff mantissa non-zero
	lsll	#1,d2		| toss sign
	bnes	1$		| not 0 or gu
	movb	#1,d3	
	tstl	d4
	beqs	3$		| zero
	movb	#2,d3
	bras	3$		| gu
1$:	swap	d2
	lsrw	#(16-11),d2	| exp to bottom of register
	cmpw	#0x7ff,d2	| inf or nan
	bnes	2$		| plain
	movw	#0x6000,d2
	movb	#4,d3
	tstl	d4
	beqs	4$		| inf
	movb	#5,d3		| nan
	bras	4$
2$:	bset	#20,d0		| hidden bit
	subqw	#1,d2
	movb	#3,d3
3$:	subw	#(1022+52), d2
4$:	rts

/*
 * reconstruct a double precision number from a record containing its pieces.
 *
 * input:
 *	d2	upper
 *	d3	lower 
 *	d6	exponent
 * output:
 *	d0/d1	result
 */

	.globl	d_pack
d_pack:
	cmpb	#4,d3	| type
	blts	1$
	orl	d1,d0
	lsll	#1,d0
	orl	#0xffe00000,d0
	bras	2$	| nan or inf
1$:	addw	#(1022+52+12),d2
	exg	d0,d2
	exg	d0,d6
	exg	d1,d3
	bsrs	d_norm
	bsr	d_rcp
	movl	d0,d6
	movl	d2,d0
	exg	d3,d1
2$:	lslw	#1,d3
	roxrl	#1,d0
	rts

/*
 * extract exponents from two double-precision numbers.
 *
 * input:
 *	d0/d1 one operand
 *	d2/d3 other operand
 *
 * output:
 *	d0/d1 mantissa, waiting for hidden bit to be turned on
 *	d2/d3 other mantissa, likewise
 *	d6	exponent from d2/d3
 *	d7	exponent for d0/d1
 *
 * destroys d4
 */

d_exte:
	moveq	#11,d4	| size of exponent
	roll	d4,d0
	roll	d4,d2
	roll	d4,d1
	roll	d4,d3
	movl	#0x7ff,d6
	movl	d6,d7
	andl	d2,d6
	eorl	d6,d2
	movl	d7,d4
	andl	d3,d4
	eorl	d4,d3
	lsrl	#1,d2
	orl	d4,d2
	| end transformation of larger
	movl	d7,d4
	andl	d0,d7
	eorl	d7,d0
	andl	d1,d4
	eorl	d4,d1
	lsrl	#1,d0
	orl	d4,d0
	| end transformation of smaller
	rts

/*
 * normalize a double-precision number
 *
 * input:
 *	d2/d3 mantissa
 *	d6    exponent
 */
d_norm:
	tstl	d2	| upper is non-zero
	bnes	1$
	cmpw	#32,d6
	blts	2$	| about to be denormalized
	subw	#32,d6
	exg	d3,d2	| shift 32
	tstl	d2
	beqs	4$	| if result == 0
1$:	bmis	3$	| if already normalized
2$:	lsll	#1,d3	| normalize
	roxll	#1,d2
	dbmi	d6,2$	| loop until normalized
	dbpl	d6,3$	| make sure d6 decremented
3$:	rts
4$:	movw	#-2222,d6	| exp == 0 for zero
	rts


/*
 * round, check for over/underflow, and pack in the exponent.
 * d_nrcp does one normalize and then calls d_rcp.
 * d_rcp rounds the double value and packs the exponent in,
 * catching infinity, zero, and denormalized numbers.
 * d_usel puts together the larger argument.
 *
 * input:
 *	d2/d3	mantissa (- if norm)
 *	d6	biased exponent
 *	(need sign, sticky)
 * output:
 *	d2/d3	most of number,	no sign or hidden bit,
 *		waiting to shift sign in.
 * other:
 *	d4	lost
 *	d5	unchanged
 */

d_nrcp:
	tstl	d2
	bmis	d_rcp	| already normalized
	subqw	#1,d6
	lsll	#1,d3	| do extra normalize (for mul/div)
	roxll	#1,d2
d_rcp:
	tstw	d6
	bgts	2$
	| exponent is negative; denormalize before rounding
	cmpw	#-53,d6
	blts	dsigned0| go all the way with zero
	negw	d6
1$:	lsrl	#1,d2	| denormalize
	roxrl	#1,d3
	bccs	1f	| Check for bit passing out forever.
	bset	#0,d3	| Stick it back on the end.
1:	dbra	d6,1$
	clrw	d6
	| round
2$:	addl	#0x400,d3
	bccs	testeven | Branch if round did not overflow lower part.
	addql	#1,d2	| carry
	bccs	testeven | Branch if round did not overflow significand.
	roxrl	#1,d2
	roxrl	#1,d3
	addqw	#1,d6
	bras	checkbig

testeven:		| Test for ambiguous case to force round to even.
	movw	#0x7ff,d4 | d4 gets rounding mask.
	andw	d3,d4	| d4 gets extra bits left after rounding.
	bnes	checkbig | Branch if it wasn't ambiguous case.
	bclr	#11,d3	| Ambiguous case: force round to even.

checkbig:
	cmpw	#0x7ff,d6
	bges	drcpbig
d_usel:
	| rebuild answer
	movl	#0xfffff800,d4
	andl	d4,d3
	andl	d2,d4
	eorl	d4,d2
	orl	d2,d3
	movl	d4,d2
	lsll	#1,d2
	|bcss	4$
	bcss	cout			| Branch if carry out occurred.
	cmpw	#0x7ff,d6
	beqs	4$
	clrw	d6
4$:	
dshiftright:			| Double shift right to pack.
	moveq	#11,d4
	rorl	d4,d3
	orw	d6,d2
	rorl	d4,d2
	rts
dsigned0:	clrl	d2
	clrl	d3
	rts
drcpbig:movl	#0xffe00000,d2	| infinity
	clrl	d3
	rts

cout:
	tstw	d6
	bnes	dshiftright	| Branch if number was not subnormal.
	movw	#1,d6		| Subnormal rounded to normal so fix exp.
	bras	dshiftright

/*
 *	single-precision coersions
 *	translated to SUN idiom 11 March 1983 rt
 */

	NSAVED	 = 4*4	| save registers d1-d4
	SAVEMASK = 0x7c00
	RESTMASK = 0x003e

/*
 * single floating to integer conversion:
 * input:
 *	d0	floating value
 * output:
 *	d0	integer value
 */

RTENTRY(Ffixis)
	moveml	#SAVEMASK,sp@-
	movl	d0,d1
	jbsr	f_unpk
	bsrs	i_pack
	movl	d1,d0
	moveml	sp@+,#RESTMASK
	RET

/*
 * single integer to floating conversion:
 * input:
 *	d0	integer value
 * output:
 *	d0	floating value
 */

RTENTRY(Ffltis)
	moveml	#SAVEMASK,sp@-
	movl	d0,d1
	bsrs	i_unpk
	jbsr	f_pack
	movl	d1,d0
	moveml	sp@+,#RESTMASK
	RET

/*
 * utility routines for converting between integers and
 * the unpacked-floating-point format in registers.
 */

i_unpk:
	clrw	d2	| exponent
	moveq	#3,d3	| type: normal number, sign: +
	tstl	d1
	bpls	1$
	negl	d1	| (largest neg number is ok here)
	bset	#15,d3	| sign flag
1$:	clrl	d0	| upper
	rts

i_pack:
	cmpb	#4,d3
	bges	3$	| infinity or nan
	bsrs	gnsh	| shift by exponent
	orw	d2,d0
	tstl	d0
	bnes	3$	| too big or bits shifted out
	tstl	d1
	bmis	3$	| too negative
	tstw	d3	| sign
	bpls	2$
	negl	d1	| change sign
2$:	rts

3$:	movl	#0x80000000,d1
	rts

/*
 * shift the mantissa by the amount in the exponent.
 * if result will not fix in 64 bits, the exponent is left non-zero,
 * garbage in upper/lower.
 * input:
 *	d0-d3	unpacked record
 * output:
 *	d0-d3	unpacked record
 *	d2.l	zero if shift could be done.
 */

 gnsh:
	moveq	#32,d4
	tstw	d2	| exponent
	bmis	3$	| right shift
	| left shift
1$:	cmpw	d4,d2
	blts	2$	| simple left
	tstl	d0
	bnes	7$	| too big
	subw	d4,d2
	exg	d1,d0	| shift 32
	bras	1$
2$:	roll	d2,d0
	roll	d2,d1
	moveq	#1,d4
	asll	d2,d4
	subql	#1,d4	| form mask
	movl	d0,d2
	andl	d4,d2	| overflow part
	bnes	7$
	andl	d1,d4	| between parts
	eorl	d4,d1	| clear bottom
	orl	d4,d0	| into top
	bras	6$	| and go

	| shift right
3$:	negw	d2
4$:	cmpw	d4,d2
	blts	5$
	movl	d0,d1	| shift 32
	clrl	d0
	subw	d4,d2
	bras	4$
5$:	moveq	#1,d4
	asll	d2,d4
	subql	#1,d4	| form mask
	notl	d4
	andl	d4,d1	| lower, 0
	andl	d0,d4	| upper, 0
	eorl	d4,d0	| 0, mid
	orl	d0,d1	| lower, mid
	rorl	d2,d4	| 0, upper
	rorl	d2,d1	| mid, lower
	movl	d4,d0	| upper
6$:	clrl	d2
7$:	rts

/* truncate to a whole number: assumes unpacked record */
	.globl	g_int
g_int:	tstw	d2
	bmi	gnsh
	rts

/*
 *	single-precision utility operations
 *
 *	copyright 1981, 1982 Richard E. James III
 *	translated to SUN idiom 11 March 1983 rt
 */

	NSAVED	 = 3*4	| save registers d2-d4
	SAVEMASK = 0x3c00
	RESTMASK = 0x003c

/*
 * unpack a single-precision number into the unpacked record format:
 *	d0/d1	mantissa
 *	d2.w	exponent
 *	d3.w	sign in upper bit
 *	d3.b	type ( zero, gu, plain, inf, nan )
 */
f_unpk:
	movl	d1,d3
	swap	d3	| sign in bit 15
	lsll	#1,d1	| toss sign
	roll	#8,d1
	clrw	d2
	movb	d1,d2	| exponent
	bnes	1$	| not gu or zero
	movb	#1,d3
	tstl	d1
	beqs	3$
	movb	#2,d3	| gu -- gradual underflow unnormalized
	bras	3$
1$:	cmpb	#255,d2	| inf or nan
	bnes	2$	| nope, plain number
	movw	#0x6000,d2
	clrb	d1	| erase exponent
	movb	#4,d3	| infinity
	tstl	d1
	beqs	4$
	movb	#5,d3	| nan
	bras	4$
2$:	movb	#1,d1	| hidden bit
	subqw	#1,d2
	movb	#3,d3
3$:	subw	#(126+23),d2
4$:	rorl	#1,d1
	lsrl	#8,d1
	clrl	d0
	rts

/*
 * reconstruct a single precision number from its pieces
 * returns packed value in d0
 */

f_pack:
	movw	d2,d4	| exponent
	cmpb	#4,d3	| is type inf or nan ?
	blts	1$	| no, then branch
	orl	d0,d1
	orl	#0x7f800000,d1	| exponent for inf/nan
	lsll	#1,d1
	bras	6$
1$:
	clrb	d2	| for sticky
	tstl	d0
	beqs	3$
	| shift from upper into lower
2$:
	orb	d1,d2	| sticky
	movb	d0,d1
	addqw	#8,d4	| adjust exponent
	rorl	#8,d1
	lsrl	#8,d0
	bnes	2$	| loop until top == 0
3$:	movl	d1,d0
	beqs	6$
	| find top bit
	bmis	5$
4$:	subqw	#1,d4	| adjust exponent
	lsll	#1,d0	| normalize
	bpls	4$
5$:	addw	#(126+23+9),d4
	tstb	d2
	beqs	7$	| Branch if no sticky.
	bset	#0,d0	| Turn on sticky bit.
7$:
	bsrs	f_rcp
	rorl	#8,d0
	movl	d0,d1
6$:	lslw	#1,d3
	roxrl	#1,d1	| append sign
	rts


/*
 * round, check for over/underflow, and pack in the exponent.
 */
	.globl	f_rcp
f_rcp:
	tstl	d0
	bmis	f_rcfast
	| do extra normalize ( for mul/div)
	subqw	#1,d4
	lsll	#1,d0	| do one normalize
f_rcfast:	
	tstw	d4
	bgts	2$
	| underflow
	cmpw	#-24,d4
	blts	rcpzero
	negb	d4
	addqb	#1,d4
	movl	d1,sp@-		| Save d1 on stack.
	clrl	d1		| d1 gets 0.
	bset	d4,d1		| For n bit shift, d1 gets 2**n.
	subql	#1,d1		| d1 gets 2**n -1, an n bit field.
	andl	d0,d1		| d1 gets bits to be shifted away.
	beqs	1f		| Branch if all zero.
	bset	d4,d0		| Sticky lsb for bits to be lost.
1:
	movl	sp@+,d1		| Restore d1.
	lsrl	d4,d0	| denormalize
	clrw	d4	| exp == 0
2$:	addl	#0x80,d0	| crude round
	bccs	stesteven
	| round overflowed
	roxrl	#1,d0
	addqw	#1,d4
	bras	scheckbig
stesteven:
	tstb	d0	| Check extra bits after round.
	bnes	scheckbig | Branch if round was not ambiguous.
	bclr 	#8,d0	| Force round to even.
scheckbig:
	cmpw	#0xff,d4	| adjust exponent
	bges	rcpinf
	lsll	#1,d0	| toss hidden
	|scs	d0	| no hidden implies zero or denormalized
	|andb	d4,d0
	|rts
	bccs	rcpsubnorm | Branch if no i bit found, implying zero or subnorm.
	movb	d4,d0	| d0 gets exponent.
	bnes	rcprts	| Branch if exp was not zero, implying normal result.
	movb	#1,d0	| Result was subnormal before round, normal after,
			| so adjust exponent accordingly.
rcprts:
	rts
rcpsubnorm:		| No carry out from shift implies zero or subnormal
			| result after rounding.
	clrb	d0	| Set minimum exponent.
	rts

rcpzero:	
	clrl	d0
	rts
rcpinf:	
	movl	#0xff,d0	| infinity
	rts


/*
 *	ieee double floating compare
 *	copyright 1981, Richard E. James III
 *	translated to SUN idiom 30 March 1983 rt
 */

/*
 *	entry conditions:
 *	    first argument in d0/d1
 *	    second argument on stack
 *	exit conditions:
 *	    result in cc -- carry flag set if either a NAN
 *	problems:
 *	    unordered cases (e.e.: projective infinities and NANs)
 *	    produce random results.
 *	    A NAN, however, does compare not equal to anything.
 *
 *	register conventions:
 *	    d0/d1	first operand
 *	    d2/d3	second operand
 *	    d4		scratch
 */
	SAVEMASK = 0x3800	| registers d2-d4
	RESTMASK = 0x1c
	NSAVED   = 3*4		| 6 registers * sizeof(register)
	CODE	 = NSAVED

RTENTRY(Fcmpi)
	subqw	#2,sp	| save room for result
|	save registers and load operands into registers
	moveml	#SAVEMASK,sp@-
	movl	ARG2PTR@+,d2
	movl	ARG2PTR@ ,d3
	| we are now set up.
	movl	d2,d4
	andl	d0,d4		| compare signs
	|bmis	nbothmi
	bpls	nbothmi
	exg	d0,d2		| both minus
	exg	d1,d3
nbothmi:cmpl	d2,d0		| main compare
	bnes	gotcmp		| got the answer
	movl	d1,d4
	subl	d3,d4		| compare lowers
	beqs	gotcmp		| entirely equal
	roxrl	#1,d4
	andb	#0xa,cc		| clear z, in case differ by 1 ulp
gotcmp:	andb	#0xe,cc		| clear carry
	movw	cc,sp@(CODE)
	lsll	#1,d0
	lsll	#1,d2
	cmpl	d2,d0
	bccs	4$
	exg	d0,d2		| find larger in magnitude
4$:	cmpl	#0xffe00000,d0
	blss	6$		| no nan
	movw	#1,sp@(CODE)	| c, nz
	bras	8$		| one was a nan
6$:	orl	d1,d0
	orl	d2,d0
	orl	d3,d0
	bnes	8$
	movw	#4,sp@(CODE)	| -0 == 0
	| done, now go
8$:	moveml	sp@+,#RESTMASK	| put back saved registers
	movw	sp@+,cc		| install condition code
	RET


/*
 *	ieee double floating add
 *	copyright 1981, Richard E. James III
 *	translated to SUN idiom 10 March 1983 rt
 */

/*
 *	entry conditions:
 *	    first argument in d0/d1
 *	    second argument on stack
 *	exit conditions:
 *	    result (8 bytes) in d0/d1
 *
 *	register conventions:
 *	    d0/d1	smaller operand (d0=most significant)
 *	    d2/d3	larger operand
 *	    d4		11 or mask of 11 bits
 *	    d5		signs: sign of .w = sign of answer
 *			       sign of .b = comparison of signs
 *	    d6		exponent of larger
 *	    d7		exponent of smaller
 */
	SAVEMASK = 0x3f00	| registers d2-d7
	RESTMASK = 0xfc
	NSAVED   = 6*4		| 6 registers * sizeof(register)

RTENTRY(Fsubi)
|	save registers and load operands into registers
	moveml	#SAVEMASK,sp@-	| registers d2-d7
	movl	ARG2PTR@+,d2
	movl	ARG2PTR@ ,d3
	bchg	#31,d2
	jra	adding
RTENTRY(Faddi)
|	save registers and load operands into registers
	moveml	#SAVEMASK,sp@-	| registers d2-d7
	movl	ARG2PTR@+,d2
	movl	ARG2PTR@ ,d3
adding:
	| extract signs
	asll	#1,d0	| sign ->c
	scs	d4	| c -> d4
	asll	#1,d2
	scs	d5
	| compare and exchange to put larger in d0/d1
	cmpl	d2,d0
	blss	1$
	exg	d0,d2
	exg	d1,d3
	exg	d4,d5
1$:	extw	d5	| sign of larger
	eorb	d4,d5	| comparison of signs
	| extract exponents
	jbsr	d_exte	| larger ->d2/d3,d6; smaller ->d0/d1,d7
	tstw	d7
	bnes	2$	| not zero or denormalized
	| here, smaller is zero or is denormalized
	movl	d0,d4
	orl	d1,d4
	jeq	signofzero | if smaller == 0 use larger
	| (sign of 0-0 unpredictable)
	lsll	#1,d1
	roxll	#1,d0
	tstw	d6	| larger exp
	bnes	3$	| not gradual underflow
	lsll	#1,d3
	roxll	#1,d2
	bras	addorsub | both gradual-underflow, no hidden or align needed
2$:	bset	#31,d0	| add hidden bit
3$:	cmpw	#0x7ff,d6
	jeq 	a_ovfl	| inf/nan
	bset	#31,d2
	| align smaller
	| shift-by-eight loop
	subw 	d6,d7
	negw	d7	| d7 = difference of exponents
	cmpw	#16,d7
	jge	rsge16	| Branch if shift of 16 or more.
rs015:			| Right shift 0..15.
	subqw	#8,d7
	blts	5$	| exit loop when difference <8
	
	tstb	d1
	beqs	99$	| Branch if no bits to lose in shift.
	bset	#8,d1	| Turn on the sticky bit if any bits will be lost.
99$:
	movb	d0,d1	| shift eight bits down
	rorl	#8,d1
	lsrl	#8,d0
	bras	rs015
5$:	addqw	#7,d7
	bmis	addorsub
	tstb	d1
	beqs	98$
	bset	#8,d1	| Turn on sticky bit.
98$:
6$:	lsrl	#1,d0
	roxrl	#1,d1
	dbra	d7,6$	| final  part of alignment
addorsub:
	| decide whether to add or subtract
	tstb	d5	| compare signs
	bmis	diff
	| add them
	addl	d1,d3	| sum
	addxl	d0,d2
	bccs	endas	| no c, ok
	roxrl	#1,d2
	roxrl	#1,d3
	addqw	#1,d6
	cmpw	#0x7ff,d6
	blts	endas	| no overflow
	jra	a_geninf
	
rsge16:			| Right shift 16 or more.
	cmpw	#32,d7
	blts	rs1631	| Branch if shift is 16..31.
	cmpw	#64,d7
	blts	rs3263	| Branch if shift is 32..63.
	clrl	d0	| Top will be zero.
	moveq	#1,d1	| Bottom will be sticky.
	bras	addorsub
rs3263:			| Shift 32.
	tstl	d1
	beqs	1$
	bset	#0,d0   | Sticky bit on.
1$:
	movl	d0,d1
	clrl	d0
	subw	#32,d7
	cmpw	#16,d7
	blts	rs015	| Branch if shift < 16.
rs1631:			| Shift 16.
	tstw	d1	
	beq	2$	| Branch if no bits in D.
	bset	#16,d1	| Turn on sticky bit in C.
2$:
	clrw	d1	| d1 gets Cs,0.
	movw	d0,d1	| d1 gets Cs,B.
	swap	d1	| d1 gets B,Cs.
	clrw	d0	| d0 gets A,0.
	swap	d0	| d0 gets 0,A.
	subw	#16,d7
	jra	rs015
	

		| subtract then
diff:	subl	d1,d3	| subtract lowers
	subxl	d0,d2	| subtract uppers
	bccs	9$
	| cancelled down into 2nd word, but got wrong sign
	notw	d5	| flip result sign
	negl	d3
	negxl	d2	| negate value
9$:	bnes	subrenorm | Branch if result nonzero.
	tstl	d3
	bnes	subrenorm | Branch if result nonzero.
	clrw	d5	| Exact zero result has positive sign.
subrenorm:		| Renormalize result after cancellation.	
	jbsr	d_norm	
	| rejoin, round
endas:	jbsr	d_rcp	| round, check, and pack
assgn:	lslw	#1,d5	| get sign
	roxrl	#1,d2	| put in sign

	| answer is now in d2/d3: put in d0/d1
	movl	d2,d0
	movl	d3,d1
asexit:	| restore registers and split
	moveml	sp@+,#RESTMASK
	RET

| EXCEPTION CASES
signofzero:		| Set up proper sign for exact zero.
	tstb	d5
	beqs	useln	| Branch if signs equal: either will do.
	tstw	d6
	bnes	useln	| Branch if not zero or subnormal.
	tstl	d2
	bnes	useln	| Branch if subnormal.
	tstl	d3
	bnes	useln	| Branch if subnormal.
	clrw	d5	| Signs unequal so set positive.

useln:	tstw	d6
	beqs	usel	| Branch if subnormal: don't set i bit.
	bset	#31,d2  | Set i bit of normal number. 
usel:	jbsr	d_usel	| use the larger
	bras	assgn

| larger exponent = 1-23
a_ovfl:	movl	d2,d4	| larger mantissa
	orl	d3,d4
	bnes	usel	| larger = nan, use it
	cmpw	d6,d7	| exps
	bnes	usel	| larger=inf and smaller=number
	| (need nan...)
	tstb	d5	| comparison of signs
	bpls	usel	| inf+inf=inf; inf-inf=nan
	movl	#0x7ff00001,d0	| NAN
	clrl	d1
	bras asexit
| result overflows
a_geninf:
	movl	#0xffe00000,d2
	clrl	d3
	bras	assgn

/*
 *	ieee double floating divide
 *	copyright 1981, Richard E. James III
 *	translated to SUN idiom 10 March 1983 rt
 */

/*
 *	entry conditions:
 *	    first argument in d0/d1
 *	    second argument on stack
 *	exit conditions:
 *	    result (8 bytes) in d0/d1
 *
 *	register conventions:
 *	    d0/d1	divisor
 *	    d2/d3	dividend
 *	    d4		count
 *	    d5		sign an exponent
 *	    d6		result exponent
 *	    d6/d7	quotient
 */
	SAVEMASK = 0x3f00	| registers d2-d7
	RESTMASK = 0xfc
	NSAVED   = 6*4		| 6 registers * sizeof(register)

RTENTRY(Fdivi)
|	save registers and load operands into registers
	moveml	#0x3f00,sp@-	| registers d2-d7
	movl	d0,d2
	movl	d1,d3
	movl	ARG2PTR@+,d0
	movl	ARG2PTR@ ,d1
	| save sign of result
	movl	d0,d5
	eorl	d2,d5		| sign of result
	clrl	d4		| flag for divide
	bsrs	extrem
	| compute resulting exponent
	movw	d6,d5
	subw	d7,d5
	| do top 30-31 bits of divide (d_rcp will post normalize)
	movw	#30,d4	| count 30..0
	bsr	shsub	| shift and subtract loop
	movl	d7,d6	| top of answer
	| do next 22 bits of divide
	movw	#23,d4	| count 23..0 (total = 54-55 bits)
	bsr	shsub
	| put together answer
	lsll	#8,d7	| line up bottom
	orl	d3,d2
	beqs	1$
	bset	#1,d7	| sticky bit on if remainder <> 0
1$:	lsll	#1,d7
	roxll	#1,d6
	| normalize (once), round, check result exp, pack
	movl	d6,d2	| upper
	movl	d7,d3	| lower
	movw	d5,d6	| exponent
	addw	#1023,d6	| re-bias
	jbsr	d_nrcp	| norm once, rnd, ck, pack
	bra	dmsign

/*
 * subroutine for unpacking one operand, and normalizing a denormalized number
 * input:
 *	d0/d1	number
 * output:
 *	d0/d1	mantissa
 *	d7.w	exponent
 *	z	on iff mantissa is zero_
 *
 * unchanged:
 *	d4	bottom = 0xf77
 */

unp:	movl	d0,d7	| start getting exp
	andl	#0xfffff,d0	| clear out sign and exp
	swap	d7
	lsrw	#(16-1-11),d7
	andw	d4,d7	| expondnt
	bnes	3$	| normal number
	| denormalized number or zero:
	tstl	d0	| upper
	bnes	1$
	tstl	d1	| lower
	beqs	3$	|zero
1$:	addqw	#1,d7
2$:	subql	#1,d7
	lsll	#1,d1
	roxll	#1,d0	| normalize
	btst	#20,d0
	beqs	2$	| loop until normalized
3$:	rts


/*
 * subroutine for extracting and taking care of 0/GU/INF/NAN.
 * input:
 *	d0/d1, d2/d3
 *	d4	+ for div; - for mod
 *
 * output:
 *	d0/d1 (bottom) and d2/d3 (top) converted to
 *		000XXXXX/XXXXXXXX
 *	d6	exponent of top
 *	d7	exponent of bottom
 *
 * unchanged:
 *	d5	sign
 */

extrem:
	movw	#0x7ff,d4	| mask, sign.l untouched
	exg	d2,d0
	exg	d3,d1
	bsrs	unp	| unpack top
	exg	d0,d2
	exg	d1,d3
	exg	d7,d6
	beqs	topzero	| top is zero
	cmpw	d4,d6
	beqs	topbig	| top is inf or nan
	bset	#20,d2	| set hidden bit
topzero:bsrs	unp	| unpack bottom
	beqs	botzero	| bottom is 0
	cmpw	d4,d7
	beqs	botbig	| bottom is inf or nan
	bset	#20,d0
	lsll	#1,d1	| left shift bottom
	roxll	#1,d0
	rts

/*
 * EXCEPTION HANDLING
 */

topbig:	| top is inf or nan
	bsrs	unp	| unpack bottom
	tstl	d4	
	bmis	isnan	| inf or nan / ...
	cmpw	d6,d7
	beqs	isnan	| both inf/nan
	tstl	d2	| top
	beqs	geninf	| inf / ... = inf
	bras	isnan	| nan / ... = nan

botzero:| bottom is 0
	tstl	d4
	bmis	gennan	| dmod(... 0) = nan
	orl	d2,d3	| top
	beqs	gennan	| 0/0 = nan
			| nonzero/0 = inf
	| generate infinity for answer
geninf:	movl	#0x7ff00000*2,d2	| infinity
	bras	clrbot

botbig:	tstl	d0	| bottom = nan, result = nan
	bnes	isnan
	tstl	d4
	bpls	genzero	| ... / inf = 0
	addqw	#4,sp
	bra	usetop	| dmod(top, inf) = top

	| invalid operand/operation
isnan:	cmpw	d7,d6
	bnes	1$	| exponents equal
	cmpl	d0, d2
1$:	bges	2$
	movw	d7,d6
	movl	d0,d2
2$:	swap	d2
	lslw	#(16-1-11),d6
	orw	d6,d2	| put back together
	swap	d2
	lsll	#1,d2
	cmpl	#0x7ff00000*2, d2
	bhis	gotnan	| use larger nan

gennan:	movl	#0x7ff00004*2,d2
	tstl	d4
	bpls	gotnan	| nan 4 for div
	addql	#2,d2	| nan 5 for mod
gotnan:	clrl	d5
	bras	clrbot

genzero:clrl	d2
clrbot:	clrl	d3
sign:	addqw	#4,sp
	bra	dmsign

/*
 *  the shsub subroutine does a shift-subtract loop
 * that is the heart of divide and mod.
 * the algorithm is a simple shift and subtract loop,
 * but it adds when it overshoots.
 * why not use the divs/divu instructions? That approach is slower!
 *
 * registers:
 *	d2/d3	current dividend (updated)
 *	d0/d1	divisor (unchanged)
 *	d4.w	(inpout) number of interations -1, and bit number
 *	d5/d6	-untouched-
 *	d7	quotient being devloped (ignored by mod)
 */


shsub:
	clrl	d7	| quotient
	| shift once, see if subtract needed
1$:	addl	d3,d3
	addxl	d2,d2	|(64-bit left shift)
	cmpl	d0,d2
	dbge	d4,1$	| loop while divident is small
	| tally quotient and subtract
	bset	d4,d7	| quotient bit
	subl	d1,d3
	subxl	d0,d2	| 64-bit subtract
	dbmi	d4,1$	| loop (d4) times
	| now one of three things has happeded:
	| 1. count exhausted and extra subtract done (first DB hit count)
	| 2. count exhausted in second DB
	| 3. overshot because compare didn't check lower parts
	bpls	2$	| case 2
	addl	d1,d3	| take care of overshoot
	addxl	d0,d2
	bclr	d4,d7
	tstw	d4
	dblt	d4,1$	| case 3
			| case 1
2$:	rts


/*
 * ieee double floating point mod
 *
 *	entry conditions:
 *	    first argument in d0/d1
 *	    second argument on stack
 *	exit conditions:
 *	    result (8 bytes) in d0/d1
 *
 *	register conventions:
 *	    d0/d1	divisor
 *	    d2/d3	dividend
 *	    d4		flag and count
 *	    d5		sign of top and result
 *	    d6		exponent
 *	    d7		other exponent
 */

RTENTRY(Fmodi)
|	save registers and load operands into registers
	moveml	#0x3f00,sp@-	| registers d2-d7
	movl	d0,d2
	movl	d1,d3
	movl	ARG2PTR@+,d0
	movl	ARG2PTR@ ,d1
	| save sign of result
	movl	d2,d5
	moveq	#-1,d4		| flag for mod
	bsr	extrem		| unpack and check end cases
	movw	d6,d4		| top exponent
	subqw	#1,d7
	subw	d7,d4		| number of iterations = top -bot +1
	bles	usetop		| top is the answer
	lsll	#1,d1		| adjust bottom so bot < top
	roxll	#1,d0
	movw	d7,d6		| result uses bot exponent
	subqw	#1,d6
	bsrs	shsub		| do work
usetop:	addw	#11,d6
	jbsr	d_norm		| normalize
	jbsr	d_rcp		| adjust exp, ck extremes, pack


	| common exit for divide and mod:
	| append sign
dmsign:	roxll	#1,d5		| sign -> x
	roxrl	#1, d2		| rotate in sign
	| answer is now in:
	|	d2	most significant 32 bits
	|	d3	least significant 32 bits
dmexit:	movl	d2,d0
	movl	d3,d1
	| restore registers and split
	moveml	sp@+,#RESTMASK
	RET

/*
 *	ieee double floating multiply
 *	copyright 1981, Richard E. James III
 *	translated to SUN idiom 10 March 1983 rt
 */

/*
	Revised to do correct IEEE rounding by dgh 24 Aug 84
 *	Conventions in the main multiplication section are as follows:
 *	r is the argument passed in the registers d0/d1
 *	s is the argument passed on the stack, saved in a0/a1
 *		while multiplying
 *	r1,r2,r3,r4 are the 16 bit components of r in descending order
 *      s1,s2,s3,s4 are the 16 bit components of s
 *	r is kept in d0 and d1, sometimes with words swapped
 *	s is kept in a0 and a1 unchanged
 *      the product is kept in d2/d3/d4/d5
 *	d6 contains a current partial product
 *	d7 contains a current partial product
 *	d7 contains 0 when needed for addxl
 *	a2 saves the sign and exponent of the result
 *
 *	At the end, d4 and d5 if nonzero are jammed into lsb of d3.
 */
/*
 *	entry conditions:
 *	    first argument in d0/d1
 *	    second argument on stack
 *	exit conditions:
 *	    result (8 bytes) in d0/d1
 *
 *	register conventions:
 *	    d0-d3	operands or pieces of result
 *	    d5		exponent of larger
 *	    d5		temp for multiplying
 *	    d6		sign and exponent
 *	    d7		exponent of smaller
 *	    d7		zero
 */
	SAVEMASK = 0x3fe0	| registers d2-d7,a0-a2
	RESTMASK = 0x7fc
	NSAVED   = 6*4		| 6 registers * sizeof(register)

	SAVE03	 = 0xf000	| registers d0-d3
	FETCH03  = 0x000f

RTENTRY(Fmuli)
|	save registers and load operands into registers
	moveml	#SAVEMASK,sp@- 	| registers d2-d7
	movl	ARG2PTR@+,d2
	movl	ARG2PTR@ ,d3
	| save sign of result
	movl	d0,d5
	eorl	d2,d5		| sign of result
	asll	#1,d0		| toss sign
	asll	#1,d2		| EEmmmm0
	cmpl	d2,d0
	| order operands (exponents at least)
	blss	eswap
	exg	d0,d2		| d2/d3 = larger
	exg	d1,d3
	| extract and check exponents
eswap:	jbsr	d_exte
	movw	d6,d5		| larger exp
	movl	d5,d6
	addw	d7,d6		| result exp (and sign)
	cmpw	#0x7ff,d5 	| check larger
	jeq	ovfl		| inf or nan
	tstw	d7
	jeq	ufl		| 0 or 	gradual underflow
	| set hidden bits
	bset	#31,d0
back:	bset	#31,d2
	| split mantissas into 4 pieces
	|moveml	#SAVE03,sp@-	| store eight words
	|movemw	sp@,#FETCH03	| reload one operand, spread out
	|movw	sp@(4*2),d5	| 0
	|mulu	d5,d0		| 00
	|mulu	d5,d1		|    01
	|mulu	d5,d2		|       02
	|mulu	d5,d3		|          03
	|clrl	d7		| used for addx instruction
|
|	movw	sp@(2*2),d5
|	mulu	sp@(5*2),d5	|          12
|	addl	d5,d3
|	addxl	d7,d1
|
|	movw	sp@(1*2),d5
|	mulu	sp@(6*2),d5
|	addl	d5,d3		|          21
|	addxl	d7,d1
|
|	movw	sp@,d5
|	mulu	sp@(7*2),d5
|	addl	d5,d3		|          30
|	addxl	d7,d1
|
|	movw	sp@(1*2),d5
|	mulu	sp@(5*2),d5
|	addl	d5,d2		|       11
|	addxl	d7,d0
|
|	movw	sp@,d5
|	mulu	sp@(6*2),d5
|	addl	d5,d2		|       20
|	addxl	d7,d0
|
|	swap	d0
|	movw	sp@,d5
|	mulu	sp@(5*2),d5
|	addl	d5,d1		|    10
|	addxl	d7,d0
|	swap	d0
|	addw	#(4*4),sp
|
|	| add together:
|	|	01--  in d0
|	|	-12-  in d1
|	|	--23  in d2
|	|	---34 in d3
|	movw	d1,d3
|	swap	d3		| --23
|	clrw	d1
|	swap	d1		| -1--
|	addl	d2,d3		| --23
|	addxl	d7,d0
|	addl	d1,d0		| 01--
|	| put together
|	movl	d0,d2

|		Main multiply sequence.

	movl	d2,a0		| a0 gets s1s2.
	movl	d3,a1		| a1 gets s3s4.
	movl	d6,a2		| a2 saves sign and exponent of result.

	movl	a0,d3		| d3 gets s1s2.
	swap	d3		| d3 gets s2s1.
	movw	d3,d4		| d4 gets s1.
	mulu	d1,d4		| d4 gets r4*s1.
	mulu	d0,d3		| d3 gets r2*s1.
	clrw	d2		| d2 gets 0; only gets carries in this phase.

	movl	a1,d6		| d6 gets s3s4.
	swap	d6		| d6 gets s4s3.
	movw	d6,d5		| d5 gets s3.
	jeq	s3zero		| Branch if s3=0 to avoid following.
	mulu	d1,d5		| d5 gets r4*s3.
	movw	d6,d7		| d7 gets s3.
	mulu	d0,d7		| d7 gets r2*s3.
	addl	d7,d4
	clrl	d7		| Make a zero for addx.
	addxl	d7,d3
	addxw	d7,d2
phase3:
	swap	d0		| d0 gets r2r1.
	swap	d1		| d1 gets r4r3.
	movw	a0,d6		| d6 gets s2.
	beqs	4$		| Skip following if s2=0.
	movw	d6,d7		| d7 gets s2.
	mulu	d0,d6		| d6 gets r1*s2.
	mulu	d1,d7		| d7 gets r3*s2.
	addl	d7,d4
	addxl	d6,d3
	clrw	d7
	addxw	d7,d2
4$:
	movw	a1,d6		| d6 gets s4.
	beqs	5$		| Skip if s4=0.
	movw	d6,d7	 	| d7 gets s4.
	mulu	d0,d6		| d6 gets r1*s4.
	mulu	d1,d7		| d7 gets r3*s4.
	addl	d7,d5
	addxl	d6,d4
	clrl	d7
	addxl	d7,d3
	addxw	d7,d2
5$:
	swap	d2		| Exchange order of registers which contain
	swap	d3		| all the "odd" partial products.
	swap	d4
	swap	d5
	movw	d3,d2		| It's really a 16 bit shift!
	movw	d4,d3
	movw	d5,d4
	clrw	d5

	movl	a1,d6		| d6 gets s3s4.
	swap	d6		| d6 gets s4s3.
	movw	d6,d7		| d7 gets s3.
	beqs	6$
	mulu	d0,d6		| d6 gets r1*s3.
	mulu	d1,d7		| d7 gets r3*s3.
	addl	d7,d4
	addxl	d6,d3
	clrl	d7
	addxl	d7,d2

6$:
	movl	a0,d6		| d6 gets s1s2.
	swap	d6		| d6 gets s2s1.
	movw	d6,d7		| d7 gets s1.
	mulu	d0,d6		| d6 gets r1*s1.
	mulu	d1,d7		| d7 gets r3*s1.
	addl	d7,d3
	addxl	d6,d2

	swap	d0		| d0 gets r1r2.
	swap	d1		| d1 gets r3r4.
	movw	a0,d6		| d6 gets s2.
	beqs	8$
	movw	d6,d7		| d7 gets s2.
	mulu	d0,d6		| d6 gets r2*s2.
	mulu	d1,d7		| d7 gets r4*s2.
	addl	d7,d4
	addxl	d6,d3
	clrl	d7
	addxl	d7,d2
8$:
	movw	a1,d6		| d6 gets s4.
	beqs	9$
	movw	d6,d7		| d7 gets s4.
	mulu	d0,d6		| d6 gets r2*s4.
	mulu	d1,d7		| d7 gets r4*s4.
	addl	d7,d5
	addxl	d6,d4
	clrl	d7
	addxl	d7,d3
	addxl	d7,d2
9$:
	orl	d5,d4
	beqs	10$		| Branch if no sticky bits.
	bset	#0,d3		| Set that sticky!
10$:
	movl	a2,d6		| Restore sign/exponent of result.

	subw	#1022,d6	| toss xtra bias
	jbsr	d_nrcp		| norm, rnd, ck, pack
msign:	roxll	#1,d6
	roxrl	#1,d2		| append sign
mexit: | answer is now in d2/d3: put in d0/d1
	movl	d2,d0
	movl	d3,d1
	| restore registers and split
	moveml	sp@+,#RESTMASK
	| slide down return address to pop off junk
	RET

s3zero:
	clrl	d5		| We do need to clear d5 sometime.
	jra	phase3

| EXCEPTION CASES

ovfl:	movl	d2,d5	| larger mantissa, if it is nan, use it
	orw	d7,d5	| smaller exponent
	orl	d0,d5
	orl	d1,d5	| smaller value
	beqs	m_gennan| inf * 0
	| else nan*x or inf*nonzero:
	movw	#0x7ff,d6
	jbsr	d_usel
	bras	msign

ufl:	movl	d0,d7	| mantissa of smaller
	orl	d1,d7
	beqs	signed0	| 0*number
normu:	subqw	#1,d6
	lsll	#1,d1	| adjust denormalized number
	roxll	#1,d0
	bpls	normu
	addqw	#1,d6
	jra	back
	| (if both are denormalized, answer will be zero anyway)
m_gennan:movl	#0x7ff00002,d2
	bras	mexit
signed0:clrl	d2
	clrl	d3
	bras	msign
	.globl ieeeused

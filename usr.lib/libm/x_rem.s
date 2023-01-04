        .data 
        .asciz  "@(#)x_rem.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .text
#include "fpcrtdefs.h"

| 	File x_rem.s provides the core remainder routine required for 
| trigonometric and exponential argument reduction.
| 	The arguments are X and Y; the computed results R and Q.
| They satisfy R = X - Q * Y and Q = [X/Y] rounded to the nearest integer,
| so R is an IEEE remainder and Q is the corresponding quotient.
| 	This routine only provides the core of the function.  
| X must be a normalized positive or negative number, not zero, inf, or nan,
| of no more than 64 significant bits. 
| Y must be a positive normalized number of no more than 64 significant bits.  
|      	R is returned as zero or a normalized number; Q as a 32 bit integer.
| If |Q| >= 2^30 then it will have the proper sign and its 30 low order bits
| will be correct, but bit 30 will be set to 1 to indicate positive overflow,
| 0 to indicate negative overflow.
|	This routine clobbers a0 and d0-d7, so preserve registers appropriately.
| Register usage:
|	d0/d1	contain the significand of X on input and R on output.
|	   d2   contains the sign of X, then R, in bit 31, and the unbiased
|		exponent in bits 0..15.
|		during the loop d2 is a constant zero.
|	d6/d7	contain the significand of Y on input.
|	   d5	contains the exponent of Y on input, and the cycle count
|		in the main loop.
|	   d3	contains the quotient Q on output, bit 30 indicating overflow.
|	   d4	contains the extension of R during the loop.	
|	   a0	saves d2 during the loop.

	
RTENTRY(x_rem)
	clrl	d3	| Q := 0.
	clrw	d4	| Sign extension of R is positive at first.
	negw	d5
	addw	d2,d5	| d5 gets cycle count = X.exp - Y.exp.
	bmis	negcount| Branch if cycle count negative: return X.
	subw	d5,d2	| Real remainder implies R.exp = Y.exp.
	movl	d2,a0	| a0 saves sign of X and exponent of R.
	clrw	d2	| d2 gets constant zero for addxw.
	bra	posrema	| Remainder starts out positive.
negcount:		| R < Y but might be > Y/2.
	cmpw	#-1,d5
	jne	normret	| Branch if R < Y/2.
	cmpl	d0,d6
	bnes	1$
	cmpl	d1,d7
1$:
	jcc	normret | Branch if R < Y/2.
	subw	d5,d2	| Exponent of R will be Y's.
	subqw	#1,d2	| But subtract 1 since Y will be left shifted.
	lsll	#1,d7   | Double Y since we can't halve R without
	roxll	#1,d6   | possibly losing the least sig bit.
	movl	d2,d5	| Sign of Q will be X's.
	bras	roundup

| Following main loop is a nonrestoring divide which accumulates the
| quotient Q in d3.
| The invariant assertion at the end of the loop is -Y <= R < Y.

toploop:
	lsll	#1,d1	| R := 2*R.
	roxll	#1,d0
	roxlw	#1,d4
	bpls	posrem
			| R < 0 so R := 2*R + Y.
	addl	d7,d1
	addxl	d6,d0
	addxw	d2,d4
	bras	botloop
posrem:
	addql	#1,d3	| Positive R means 1 bit in Q.
posrema:
	subl	d7,d1	| R >= 0 so R := 2*R - Y.
	subxl	d6,d0
	subxw	d2,d4
botloop:
	lsll	#1,d3	| Q := 2*Q.
	bccs	1$	| Branch if no overflow.
	cmpw	#32,d5
	blts	4$	| Branch if count has less than	32 more cycles to go.
	bset	#0,d3	| Turn on least significant bit to cycle out later.
	bras	1$
4$:	bset	#30,d3	| Turn on significant bit.
1$:
	dbf	d5,toploop | At this point -Y <= R < +Y.

	movl	a0,d2	| Restore sign of X and exponent of R.
	movl	d2,d5	| d5 gets sign of Q = sign of X.
	tstw	d4
	bpls	rpos	| Branch if R is positive.
	addl	d7,d1
	addxl	d6,d0	| R := R + Y so 0 <= R < Y.
	bras	check0:
rpos:
	addql	#1,d3	| Final bit of Q.
check0:
	tstl	d1	| At this point 0 <= R < Y.
	bnes	check	| Testing for R = 0.
	tstl	d0
	bnes	check
	tstw	d4
	beqs	fixq	| Branch on exact zero result.

check:			| At this point 0 < R < Y.
			| Remainder is not exact zero so check rounding of Q.
			| If 2*R < Y then Q is OK.
			| If 2*R = Y then round Q to even; R = +- Y/2.
			| If 2*R > Y then round Q up; R := R - Y.
	
	tstl	d0
	bmis	roundup	| R >= 0.5 so 2*R > Y for sure.
	lsll	#1,d1
	roxll	#1,d0	| Double R.
	cmpl	d0,d6
	movw	cc,d4	| d4 gets compare result.
	bnes	1$
	cmpl	d1,d7
	movw	cc,d4
1$:
	lsrl	#1,d0
	roxrl	#1,d1	| Undo the doubling of R.
	movw	d4,cc	| Restore condition codes.
	bhis	fixq	| Y > 2R so Q is ok.
	bcss	roundup	| Y < 2R so Q increment Q.
	btst	#0,d3	| Y = 2R so check Q for even.
	beqs	fixq	| Branch if Q is already even.
roundup:		| Increment Q, decrement R.
	addql	#1,d3	| Increment Q.
	bccs	2$ 
	bset	#31,d3	| Set overflow indicator.
2$:
	subl	d7,d1	| Y/2 <= R < Y so let R := R - Y.
	subxl	d6,d0	| Thus -Y/2 <= R < 0.
	negl	d1
	negxl	d0	| Negate so 0 < -R <= Y/2.
	bchg	#31,d2  | Reverse sign of remainder.
	
fixq:			| At this point -Y/2 <= R <= Y/2.
	cmpl	#0x3fffffff,d3
	bls	qsign   | Branch if Q <= 2**30 -1.
	bset	#30,d3	| Set overflow bit.
	bclr	#31,d3	| Clear sign bit.
qsign:
	tstl	d5
	bpls	1$	| Branch if quotient is positive.
	negl	d3	| Negate negative quotient.
1$:			| Normalize remainder.
	tstl 	d0
	bnes	normtest
	subw	#32,d2
	movl	d1,d0	| 32 bit normalize.
	beqs	normret	| But exact zero needn't be normalized.
	clrl	d1
normtest:
	tstl	d0
	bmis	normret | Branch if normalized.
normloop:
	subqw	#1,d2
	lsll	#1,d1
	roxll	#1,d0
	bpls	normloop | Branch if not normalized.
normret:
	RET

	.globl	_dtrigarg      
			| double dtrigarg( x, qprtr ) ;
			| double x ; int *qptr ;
_dtrigarg:
	link	a6,#0
	RTMCOUNT
	moveml	#0x3f80,sp@-
	moveml	pio2,#0xe0	| Load d5/d6/d7 = pi/2.
	movl	a6@(0x8),d0	| Load x.
	movl	a6@(0xc),d1
	bsrs	unpack
	bsr	x_rem
	movl	a6@(0x10),a0
	movl	d3,a0@		| Store *qptr.
	bsr 	pack
	moveml	sp@+,#0x1fc
	unlk	a6
	rts

	.globl	_dexparg      
			| double dexparg( x, qprtr ) ;
			| double x ; int *qptr ;
_dexparg:
	link	a6,#0
	RTMCOUNT
	moveml	#0x3f80,sp@-
	moveml	loge2,#0xe0	| Load d5/d6/d7 = pi/2.
	movl	a6@(0x8),d0	| Load x.
	movl	a6@(0xc),d1
	bsrs	unpack
	bsr	x_rem
	movl	a6@(0x10),a0
	movl	d3,a0@		| Store *qptr.
	bsrs	pack
	moveml	sp@+,#0x1fc
	unlk	a6
	rts
	
	| puts significand in d0/d1, sign/exp in d2, uses d4 for scratch.
unpack:
	roll    #4,d0           | Move sign.
        roll    #8,d0           | Move exponent.
        movw    d0,d2           | d2 gets sign and exponent.
        andl    #0xfffff000,d0  | Clear 12 bits from d0.
        andw    #0xfff,d2       | Remove junk.
        swap	d2
	bclr    #27,d2          | Clear sign bit.
        sne     d2              | d5 gets sign.
        extw	d2
	swap	d2
	roll    #4,d1           | Align d1.
        roll    #8,d1
        movw    d1,d4           | d4 gets low order of d1.
        andl    #0xfffff000,d1  | Clear 12 bits from d1.
        andw    #0xfff,d4       | d4 gets 12 low order bits of argument.
        orw     d4,d0           | Move 12 bits to d0.
|		Zero and subnormal operands aren't allowed.
|        tstw    d2
|        bnes    testmax         | Branch if not zero or subnormal.
|        tstl    d0
|        bnes    subnormal
|        tstl    d1
|        bnes    subnormal
|	rts

|subnormal:                      | Subnormal number.
|        tstl    d0
|        bmis    main            | Branch if normalized now.
|1$:     subqw   #1,d2           | Decrement exponent.
|        lsll    #1,d1           | Normalize.
|        roxll   #1,d0
|        bpls    1$              | Branch if still not normalized.
|        bras    main
|
|testmax:
|normal:
        lsrl    #1,d0           | Make room for leading bit.
        roxrl   #1,d1
        bset    #31,d0          | Set I bit.
main:
	subw	#0x3ff,d2	| Subtract bias.
	rts

pack:			| Zero and subnormal results aren't possible.
|	tstl	d1
|	bnes	testsub
|	tstl	d0
|	bne	testsub
|	movl	d2,d0
|	andl	#0x80000000,d0
|	rts			| True zero.
|testsub:
        addw	#0x3ff,d2	| Bias.
|	bgts	pnormal
|	subqw	#1,d2
|denorm:
|	lsrl	#1,d0
|	roxrl	#1,d1
|	addqw	#1,d2
|	blts	denorm
|pnormal:
	addl	#0x400,d1	| Round bit.
	bccs	2$
	addql	#1,d0		| Carry propagation.
	bccs	2$
	roxrl	#1,d0		| Carry out, so undo it.
	addqw	#1,d2		| Adjust exponent.
2$:
	lsll    #1,d1           | Clear I bit.
        roxll   #1,d0
        andw    #0xf000,d1      | Make space for extra bits.
        movw    d0,d4
        andw    #0xfff,d4       | D6 gets 12 extra bits for d1.
        orw     d4,d1
        rorl    #4,d1           | Reposition bits.
        rorl    #8,d1
        andw    #0xf000,d0      | Make space for exponent.
	tstl	d2
	bpls	1$
	addw	#0x800,d2	| Insert sign.
1$:
	orw     d2,d0           | Insert exponent.
        rorl    #4,d0           | Reposition exponent.
        rorl    #8,d0	
	rts
	
	.globl	pio2,loge2
pio2:	.long	0,0xc90fdaa2,0x2168c235
loge2:	.long	-1,0xb17217f7,0xd1cf79ac

RTENTRY(s_trigarg)
			| input d0 is single normalized
			| output d0 is remainder, zero, subnormal, or norm
			| output d1 is quotient, an integer
	moveml	#0x3f80,sp@-
	moveml	pio2,#0xe0	| Load d5/d6/d7 = pi/2.
	bsrs	s_unpack
	bsr	x_rem
	bsrs	s_pack
	movl	d3,d1		| Store quotient.
	moveml	sp@+,#0x1fc
	RET

RTENTRY(s_exparg)
			| input d0 is single normalized
			| output d0 is single normalized remainder;
			| zero or subnormal can't happen here.
			| output d1 is quotient, an integer
	moveml	#0x3f80,sp@-
	moveml	loge2,#0xe0	| Load d5/d6/d7 = loge2.
	bsrs	s_unpack
	bsr	x_rem
	bsrs	s_pack
	movl	d3,d1		| Store quotient.
	moveml	sp@+,#0x1fc
	RET

s_unpack:		| Unpack normalized d0 into d0/d1/d2 for x_rem.
	movl	d0,d2		| d2 gets sign of X.
        roll    #1,d0           | X gets sign bit; bit 0 gets junk.
        bclr    #0,d0           | Clear bit 0.
        roll    #8,d0           | Move exponent to least sig byte.
        clrw    d2
        movb    d0,d2           | d2 gets (biased) exponent.
        subw	#127,d2		| Subtract bias.
	movb    #1,d0           | Clear exponent and set i bit.
        rorl    #1,d0           | Position leading bit.
	clrl	d1		| Clear lesser bits.
	rts

s_pack:			| Pack normalized d0/d1/d2 into d0.
	addl	#0x80,d0	| Add round bit.
	bccs	align		| Don't sweat ambiguous case.
	roxrl	#1,d0		| Carry out so restore leading bit,
	addqw	#1,d2		| and adjust exponent.
align:
	roll	#1,d0		| Move implicit bit to bit 0.
	addw	#127,d2		| Add in bias.
	movb	d2,d0		| Insert exponent in d0.
	rorl	#8,d0		| Rotate exponent.
	roxll	#1,d2		| X bit gets sign.
	roxrl	#1,d0		| Insert sign.
	rts

|	.globl _getrem
|_getrem:
|			| procedure getrem( x, y : real, var r : real ;
|			| var q : integer ) ;
|	link	a6,#0
|	moveml	#0xff80,sp@-
|	movl	a6@(0x10),d0
|	movl	a6@(0x14),d1
|	bsrs	unpack
|	movl	d0,d6
|	movl	d1,d7
|	movl	d2,d5
|	movl	a6@(0x8),d0
|	movl	a6@(0xc),d1
|	bsrs	unpack
|	bsr	x_rem
|	movl	a6@(0x1c),a0
|	movl	d3,a0@	| Store q.
|	bsrs	pack
|	movl	a6@(0x18),a0
|	movl	d0,a0@
|	movl	d1,a0@(4)
|	moveml	sp@+,#0x1ff
|	unlk	a6
|	rts


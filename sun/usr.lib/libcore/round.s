	.data
	.asciz "@(#)round.s 1.1 86/09/25 Copyr 1983 Sun Micro"
	.even
	.text

| Copyright (c) 1985 by Sun Microsystems, Inc.
	| These routines are C-callable rounding routines for conversion of
	| IEEE float or double to integer.  The math library routines for
	| truncating conversion are called and the result is incremented
	| or decremented if appropriate.
	| Single pointer-to-float argument call.
	| On return d0 will contain the result.

	.globl __core_roundf, __core_roundd
	.globl fvfixis, fvfixi


	| IEEE single precision representation is

	| 1 bit|   8 bits | 23 bits  |
	| sign | exponent | mantissa |  -- 32-bit word

	| with a hidden bit
	| for 0 < e < 255, # = (-1)**s * 2**(e-127) * (1.m)
	| or # = (-1)**s * 2**(e-126) * (.1m)
	|e = 0 or e = 255 => NaN, infinity, 0, or very tiny denormalized number

__core_roundf:
        movl    sp@(4),a1
	movl	a1@,d0
	jbsr	fvfixis		| Convert float to int with truncation.
        movl    sp@(4),a1
	movl	d0,a0
	movl	a1@,d0
	movl	d0,d1
	swap	d0
	lsrw	#7,d0
	subb	#126,d0
	jeq	doit
	jmi	skippit
	cmpb	#24,d0
	jge	skippit
	negb	d0
	addb	#23,d0
	btst	d0,d1
	jeq	skippit
doit:	movl	a0,d0
	tstl	d1
	jmi	negit
	addql	#1,d0
	rts
negit:	subql	#1,d0
	rts
skippit:movl	a0,d0
	rts


	| IEEE double precision representation is

	| 1 bit|  11 bits | 20 bits  |
	| sign | exponent | mantissa |  -- lower address 32-bit word

	| 32 bits                    |
	| mantissa(least significant)|	-- higher address 32-bit word

	| with a hidden bit
	| for 0 < e < 2047, # = (-1)**s * 2**(e-1023) * (1.m)
	| or # = (-1)**s * 2**(e-1022) * (.1m)
	|e = 0 or e = 2047 => NaN, infinity, 0, or very tiny denormalized number

__core_roundd:
	movl	a5,sp@-
	movl	sp@(8),a5
	movl	a5,a1
	movl	a1@+,d0
	movl	a1@,d1
	jbsr	fvfixi		| Convert double to int with truncation.
	movl	d0,a0
	movl	a5@,d0
	movl	d0,d1
	swap	d0
	lsrw	#4,d0
	andw	#0x07FF,d0
	subw	#1022,d0
	jeq	doitd
	jmi	skipitd
	cmpw	#32,d0
	jge	skipitd
	negw	d0
	addw	#20,d0
	jpl	tstit
	movl	a1@,d1
tstit:	btst	d0,d1
	jeq	skipitd
doitd:	movl	a0,d0
	tstl	a5@
	jmi	negitd
	addql	#1,d0
	bra     retdd
negitd:	subql	#1,d0
	bra     retdd
skipitd:movl	a0,d0
retdd:  movl    sp@+,a5
	rts

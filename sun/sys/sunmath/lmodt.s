	.data
	.asciz	"@(#)lmodt.s 1.1 86/09/25 SMI"
	.even
	.text
|
|	Copyright (c) 1985 by Sun Microsystems, Inc.
|

#include "../machine/asm_linkage.h"
#undef ENTRY

div_neg_a:
	| the dividend is negative: make it positive;
	| make sure the divisor is positive, too
	| remainder takes sign of dividend
	negl	d0
	tstl	d1
	bges	1$
	    negl	d1
1$:	bsrs	div_recall
	negl	d0
	rts

divide_by_zero:
	| the divisor is zero. if we're going to dump core,
	| lets put down a link, so that adb will grot our core image
	link	a6,#0
	divu	d1, d0	| BOOM!
	unlk	a6
	rts

|
| ENTRYPOINT FOR UNSIGNED REMAINDERING
| arguments in d0, d1
| return remainder in d0, garbage in d1
|
	CCENTRY(ulmodt)
	tstl	d1
	beqs	divide_by_zero
	bras	div_recall

|
| ENTRYPOINT FOR SIGNED REMAINDERING
| receive arguments in d0, d1
| return remainder in d0, garbage in d1
|
	CCENTRY(lmodt)
	tstl	d0
	blts	div_neg_a	| dont't deal with signs here
	tstl	d1
	beqs	divide_by_zero
	bges	div_recall
	    negl	d1	| sign of divisor unimportant
div_recall:

	.globl ptwo
	| register usage
	a	=	d0
	b	=	d1
	ahi	=	d2
	x	=	ahi
	bhi	=	d3
	lo_quot = 	bhi
	bcopy	=	bhi
	shf_cnt	=	d4
	y	=	shf_cnt
	z	=	d5
	potreg	=	a0
	| save mask - assumes <ahi> = <d2> saved first in fast case,
	|	<d3-d5,a0> saved later if necessary
	SAVE	=	0x3c80
	| restore mask
	RST	=	0x013c
|
| assuming b<0x10000 is the commonest case,
| try to handle it quickly.
|
	movl	ahi,sp@-	| note ahi is x!
	movl	b,x		| check for b >= 2**16
	clrw	x
	swap	x
	bnes	long_case	| there are high-order bits in b
	| here if b is < 2**16
	movl	a,ahi
	clrw	ahi
	swap	ahi
	cmpl	b,ahi
	bges	1$
	| here if a short division will not overflow
	divu	b, a	| b < ahi !
	| get remainder and return
	clrw	a
	swap	a
	movl	sp@+,ahi
	rts

1$:	| here if the divisor is much smaller than the dividend,
	| so a straight divide is untenable.
	movl	lo_quot,sp@-
	divu	b, ahi		| division of high-order word
	movl	ahi, lo_quot	| remainder scoots down into
	movw	a, lo_quot	| ... low-order division.
	divu	b, lo_quot	| get remainder from this division
	clrw	lo_quot
	movl	lo_quot, a
	swap	a		| remainder in a
	movl	sp@+,lo_quot
	movl	sp@+,ahi
	rts

long_case:
	| general case : save the rest of the registers; x = ahi is on TOS
	movl	sp@+,ahi
	moveml	#SAVE, sp@-
	movl	b, bhi		| detect special case of power-of-two
	subql	#1, bhi
	andl	b, bhi
	beq	pot		| is b a power-of-two?
	| here if b is > 2**16, so simple division is impossible.
	| one degenerate case is for a/b <= 8, which we can do by subtraction.
	moveq	#0, shf_cnt
	movl	a,  x
	lsrl	#3, x
	cmpl	b,  x
	bhis	10$
1$:
		addql	#1, shf_cnt
		subl	b,  a
		bhis	1$		| really want bhss, or something.
	        beqs	3$		| stupid instruction set.
		addl	b, a
3$:	    | what remains is the remainder

	    moveml	sp@+,#RST
	    rts
10$:	| this is the hard case. 
	| we must shift the dividend and the divisor until the divisor
	| will fit into 16 bits. Then we can do a divide, which gives
	| us a good guess as to the quotient. The guess may be off by
	| one or two. so we correct for it at the end.
	movl	a, x
	movl	b, bcopy
	movl	#0xffff,z
	subql	#1, bcopy
11$:
	    addql	#1, shf_cnt	
	    lsrl	#1, bcopy
	    cmpl	z, bcopy
	    bhis	11$
	addql	#1, bcopy
	cmpl	z, bcopy	| did add cause a carry-out?
	blss	12$
	    lsrl	#1, bcopy
	    addql	#1, shf_cnt
12$:
	lsrl	shf_cnt, x 	| shift a by a like amount
	| divisor is now < 2**16, so division is now possible
	| dividend could not have been bigger than 2**16 * divisor,
	| so we cannot overflow.
	divu	bcopy, x
	movl	b, bcopy	| multiply back by divisor
	movl	b, z
	swap	bcopy
	mulu	x, z
	mulu	x, bcopy
	swap	bcopy
	clrw	bcopy
	addl	bcopy, z
	subl	z, a		| take trial remainder
	cmpl	a, b
	bgts	16$		| if remainder < b, we're done
	    subl	b, a	| correct remainder
16$:	
	moveml	sp@+,#RST
	rts
pot:	| divisor is a power of two: find shift count
	moveq	#0, shf_cnt
	tstw	b
	bnes	1$
	    swap	b	| bit in high-half
	    moveq	#16, shf_cnt
1$:
	lea	ptwo, potreg
	tstb	b
	bnes	2$
	    addqb	#8, shf_cnt
	    lsrw	#8, b
2$:
	addb	potreg@(0,b:w), shf_cnt
	| prepare to form quotient and remainder

	moveq	#1, x		| make remainder by masking
	lsll	shf_cnt, x
	subql	#1, x
	andl	x, a
	moveml	sp@+,#RST
	rts


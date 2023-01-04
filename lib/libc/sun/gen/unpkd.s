	.data
	.asciz	"@(#)unpkd.s 1.1 86/09/24 Copyr 1984 Sun Micro"
	.even
	.text

|	Copyright (c) 1984 by Sun Microsystems, Inc.

|
|
| subroutines to deal with unpacked double form:
| struct unpkd { short exp; unsigned mantissa[2]; };
|

	param1	=	8
	param2	=	12
	param3	=	16

	exp	=	0
	man1	=	2
	man2	=	4
	man3	=	6
	man4	=	8
	.globl __unorm
|	_unorm( up ) struct unpkd *up; 
|	{
|		while (!up->mantissa&High_order_bit){
|			up->mantissa<<=1;
|			up->exp--;
|		}
|	}
__unorm: link	a6,#0
#if PROF
	.data
1$:	.long	0
	.text
	lea	1$,a0
	jsr	mcount
#endif
	movl	d2,sp@-
	moveq	#0,d0
	moveq	#0,d1
	movl	a6@(param1),a0	| a0 addresses structure to normalize
	movw	a0@+,a1		| keep exp handy in a1
	moveq	#3,d2
	bras	2f
1:		lea	a1@(-16),a1	| think of it as shifting 16 per
2:		movw	a0@+,d0		| find first non-zero halfword
	dbne	d2,1b
	swap	d0
	tstw	d2
	bles	9f		| low-order 16 bits only
	movw	a0@+,d0		| fill in first word
	subqw	#1,d2
	bles	9f
	movw	a0@+,d1
	swap	d1
	subqw	#1,d2
	bles	1f
	movw	a0@+,d1
1:
	tstl	d0
	blts	2f		| may already be normal
1:	| 64-bit normalize loop
		subqw	#1,a1
		addl	d1,d1
		addxl	d0,d0
	bpls	1b
	bras	2f
9:	| 32-bit normalize loop
	tstl	d0
	blts	2f		| may already be normalized
1:
		subqw	#1,a1
		addl	d0,d0
	bpls	1b
2:
	movl	d1,a0@-
	movl	d0,a0@-
	movw	a1,a0@-
	movl	sp@+,d2
	unlk	a6
	rts

	.globl	__umult3
|
|	_umult3( a, b, c ) struct unpkd *a, *b, *c;
|	{
|		*c = *a * *b;
|	}

__umult3: link a6,#-4
#if PROF
	.data
1$:	.long	0
	.text
	lea	1$,a0
	jsr	mcount
#endif
| 	do a quad-word fractional multiply.
|	then adjust the exponent and renormalize.
|
| use:	d0: accumulator
|	d1: develop products
|	d2: collect carry-outs
|	d3: zero: help with carry-outs
|	d4: first word, first operand
|	d5: second word, first operand
|	d6: first word, second operand
|	d7: second word, second operand
|	a0: address of first parameter, later result
|	a1: address of second parameter
	moveml	#0x3f00,sp@-
	clrw	d2
	clrw	d3
	movl	a6@(param1),a0
	movl	a0@(man1),d4
	movl	a0@(man3),d5
	movl	a6@(param2),a1
	movl	a1@(man1),d6
	movl	a1@(man3),d7
	| we should to develop ALL 16 BYTES of the product, I think,
	| because lower bits could affect the bits we wish to keep.
	| we are going to skip a few steps, though (and 3 multiplies),
	| and round up from 16 bits below the least significant of the
	| result.
	| I could be wrong on this.
	clrl	d0
	movw	d5,d0
	beqs	1f
	tstw	d6
	beqs	1f
	mulu	d6,d0	| (param1 man4) * (param2 man2)
1:	movw	d4,d1
	beqs	1f
	tstw	d7
	beqs	1f
	mulu	d7,d1	| (param1 man2) * (param2 man4)
	addl	d1,d0
	addxw	d3,d2	| catch overflow
1:	| get 3rd halfword in position
	swap	d5
	swap	d7
	movw	d5,d1
	beqs	1f
	tstw	d7
	beqs	1f
	mulu	d7,d1	| (param1 man3) * (param2 man3)
	addl	d1,d0
	addxw	d3,d2
1:	movw	d2,d0
	swap	d0
	addql	#1,d0	| ROUND HERE
	clrw	d2	| reset overflow bucket
	| get most, least significant halfwords in position
	swap	d4
	swap	d5
	swap	d6
	swap	d7

	movw	d7,d1
	beqs	1f
	mulu	d4,d1	| (param1 man1) * (param2 man4)
	addl	d1,d0
	addxl	d3,d2
1:	movw	d5,d1
	beqs	1f
	mulu	d6,d1	| (param1 man4) * (param2 man1)
	addl	d1,d0
	addxl	d3,d2
1:	| swap high & low words
	swap	d4
	swap	d5
	swap	d6
	swap	d7
	movw	d5,d1
	beqs	1f
	tstw	d6
	beqs	1f
	mulu	d6,d1	| (param1 man3) * (param2 man2)
	addl	d1,d0
	addxl	d3,d2
1:	movw	d4,d1
	beqs	1f
	tstw	d7
	beqs	1f
	mulu	d7,d1	| (param1 man2) * (param2 man3)
	addl	d1,d0
	addxl	d3,d2
1:
	| now looks like:
	|     d4,d6:     (high halfword) | (2nd halfword )
	|     d5,d7:     (4th halfword ) | (3rd halfword )
	| swap high halfword into position
	swap	d4
	swap	d6
	addl	#0x00008000,d0	| ROUND AGAIN
	addxl	d3,d2
	movw	d2,d0
	swap	d0
	clrw	d2
	| now looks like:
	|     d4,d6:     (2nd halfword ) | (high halfword)
	|     d5,d7:     (4th halfword ) | (3rd halfword )
	movw	d7,d1
	beqs	1f
	mulu	d4,d1	| (param1 man1) * (param2 man3)
	addl	d1,d0
	addxl	d3,d2
1:	movw	d5,d1
	beqs	1f
	mulu	d6,d1	| (param1 man3) * (param2 man1)
	addl	d1,d0
	addxl	d3,d2
1:	| third and forth halfwords aren't used any more, so copy
	| the second halfword (currently out of position) down onto them
	movl	d4,d5
	movl	d6,d7
	swap	d5
	swap	d7
	movw	d5,d1
	beqs	1f
	tstw	d7
	beqs	1f
	mulu	d7,d1	| (param1 man2) * (param2 man2)
	addl	d1,d0
	addxl	d3,d2
1:	movw	d0,a6@(-2)
	movw	d2,d0
	swap	d0
	clrw	d2
	movw	d7,d1
	beqs	1f
	mulu	d4,d1	| (param1 man1) * (param2 man2)
	addl	d1,d0
	addxl	d3,d2
1:	movw	d5,d1
	beqs	1f
	mulu	d6,d1	| (param1 man2) * (param2 man1)
	addl	d1,d0
	addxl	d3,d2
1:	movw	d0,a6@(-4)
	movw	d2,d0
	swap	d0
	movw	d4,d1
	mulu	d6,d1	| (param1 man1) * (param2 man1)
	addl	d1,d0
	movl	a6@(-4),d1
	roxrl	#1,d0
	roxrl	#1,d1
	movw	a0@,d2
	addw	a1@,d2
	addqw	#2,d2
	| have mantissa in d0/d1
	| have exponent in d2
	| now normalize.
	tstl	d0
	blts	2f
1:	subqw	#1,d2
	addl	d1,d1
	addxl	d0,d0
	bpls	1b
2:	| return results
	movl	a6@(param3),a0
	movw	d2,a0@+
	movl	d0,a0@+
	movl	d1,a0@

	moveml	sp@+,#0x00fc
	unlk	a6
	rts

	.globl	__uentier
| take the current (normalized) unpacked number
| and denormalize it so that the one's digit is in the least significant bit.
| in other words, shift right by 63-(exponent), then round.
| Since we end our shift with a dbra instruction, which terminates the loop
| on a 0 -> -1 transition, we must bias our iteration count by -1. Thus the
| magical '62' in what follows.
|
__uentier:
	link	a6,#0
#if PROF
	.data
1$:	.long	0
	.text
	lea	1$,a0
	jsr	mcount
#endif
	| register usage:
	|	d0 : mantissa
	|	d1 : more mantissa
	|	d2 : exponent
	|	a0 : argument pointer
	movl	a6@(param1),a0
	movw	d2,sp@-
	movl	a0@(man1),d0
	movl	a0@(man3),d1
	movw	#62,d2
	subw	a0@,d2
	blts	2f
	cmpw	#32,d2
	blts	3f
		movl	d0,d1
		moveq	#0,d0
		subw	#32,d2
3:	cmpw	#16,d2
	blts	1f
		movw	d0,d1
		clrw	d0
		swap	d0
		swap	d1
		subw	#16,d2
1:	lsrl	#1,d0
	roxrl	#1,d1
	dbra	d2,1b
	bccs	2f
		addql	#1,d1	| carry in
		bccs	2f
		 	addql	#1,d0 | carry out
			| further carries IMPOSSIBLE
2:	| exponent is garbage now.
	| put back mantissa.
	movl	d0,a0@(man1)
	movl	d1,a0@(man3)
	| restore register and go.
	movw	sp@+,d2
	unlk	a6
	rts

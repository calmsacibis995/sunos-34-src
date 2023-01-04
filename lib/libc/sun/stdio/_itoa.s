|
	.data
	.asciz	"@(#)_itoa.s 1.1 86/09/24 Copyr 1984 Sun Micro"
	.even
	.text

|	Copyright (c) 1984 by Sun Microsystems, Inc.

|
|	convert a 32-bit binary integer into a decimal character string.
|	do it the obvious way -- with successive divides by 10.
|	return updated character pointer.
|
	| operands
	valparam	=	8
	stringparam	=	12

	| registers
	ten		=	d0	| keep "10" in low-order 16 bits
	sign		=	d0	| keep sign of operand in highest bit
	zero		=	d0	| later, keep '0' there
	value		=	d1	| keep value here...but
	vhi		=	d2	| keep high-order word here.
	ndigit		=	d3	| number of digits developed
	mask		=	d4	| 0xffff
	junk		=	d5
	string		=	a0	| output char *
	stemp		=	a1	| temp pointer
	| size of work area
	NSAVED		=	4	| save d2,d3,d4,d5
	frame		=	10+(4*NSAVED)
	tmpstring	= 	-4*(NSAVED+1)

	.globl	__itoa, __utoa
__utoa:	
	link	a6,#-frame	| push workarea, save registers.
#ifdef PROF
	.data
1$:	.long	0
	.text
	lea	1$,a0
	jsr	mcount
#endif PROF
	moveml	#0x003c,a6@(-16) | d2,d3,d4,d5
	subl	ndigit,ndigit	| clear ndigit, and the X bit
	movl	a6@(stringparam),string | get parameters
	movl	a6@(valparam),value
	bras	1f
__itoa:	
	link	a6,#-frame	| push workarea, save registers.
#ifdef PROF
	.data
1$:	.long	0
	.text
	lea	1$,a0
	jsr	mcount
#endif PROF
	moveml	#0x003c,a6@(-16) | d2,d3,d4,d5
	subl	ndigit,ndigit	| clear ndigit, and the X bit
	movl	a6@(stringparam),string | get parameters
	movl	a6@(valparam),value
	bgts	1f
	    | negative case: positivize
	    negl	value
1:	roxrl	#1,sign	| save off sign
	movw	#10,ten
	lea	sp@(-1),stemp
	movl	#0xffff,mask
longdivide:
	| as long as "value" is > 0xa0000, we must do long division
	movl	value,vhi
	swap	vhi
	cmpw	vhi,ten	| compare high word of value with "10"
	jhi	shortdivide | WAS JGE, then JCC
	| must do long division here.
	andl	mask,vhi
	divu	ten,vhi	| quotient in lo, remainder in high
	movw	vhi,junk | save quotient
	movw	value,vhi | prepare for 2nd division
	divu	ten,vhi	| quotient in lo, remainder in high
	movl	junk,value
	swap	value
	movw	vhi,value | quotient back in value
	swap	vhi	| remainder HERE
	movb	vhi,stemp@+
	addqw	#1,ndigit
	jra	longdivide
shortdivide: | only interesting bits in low-order part
	divu	ten,value	| quotient in low, remainder in high
	movl	value,vhi
	swap	vhi
	movb	vhi,stemp@+
	addqw	#1,ndigit
	andl	mask,value
	jne	shortdivide
| developed all digits, now shovel to result area.
	tstl	sign
	bges	1f
		movb	#0x2d,string@+	| '-'
1:	movb	#0x30,zero	| '0'
	subqw	#1, ndigit
copyloop:
	movb	stemp@-,value
	addb	zero,value
	movb	value,string@+
	dbra	ndigit,copyloop
	movb	#0,string@	| null terminate
| done -- restore registers and go
	movl	string,d0	| result returned
	moveml	a6@(-16),#0x3c	| d2,d3,d4,d5
	unlk	a6
	rts

        .data 
        .asciz  "@(#)s_atan.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .text

|	Copyright (c) 1983 by Sun Microsystems, Inc.

#include "DEFS.h"

        .even 
        .data
	.globl	fvatanis
fvatanis:        jsr     sky_switch:L
                jmp     Fatanis:L
                jmp     Satanis:L
        .even
        .text
 
| single precison atan function adapted from approximation # 5093 in Hart.

	SAVEALL	 = 0x3F80	| registers a0,d2-d7
	RESTALL	 = 0x01fc	

ONE:	.long	0x3F800000	| 1.000000e+00
PIO2:	.long	0x3FC90FDB	| 1.570796e+00
PIO2b:	.long	0x333bbd2e	| pi/2 = PIO2 - PIO2b
PIO4:	.long	0x3f490FDB	| 1.570796e+00/2
P2:	.long	0x69b2ca49 	| 0.41288437160814
P3:	.long	0x5b275dd7 	| 0.35606943612694
P4:	.long	0x0d2ddbd8 	| 5.1480999222358d-02
Q1:	.long	0x4f46199b 	| 0.30966339155542
Q2:	.long	0x73edd1ff	| 0.45284759995657
Q3:	.long	0x2d7906e9 	| 0.17762797546357
|Q4:	.long	0x04000000 	| 1.5625000000000d-02 (implemented by lsl 6)

| 	d2/d3/d4	used for scratch by fixed point routines.
|	d5		holds a**2
|	d6		holds a, the argument to the arctan approx
|	d7		holdx x, the original argument.
|	a0		holds p, part of the approximation p/q.
	
	RTENTRY(Fatanis)
	moveml	#SAVEALL,sp@-			| save d2-d7
	movl	d0,d1
	movl	d1,d7				| save 
	jsr	f_tst				| test the argument
	addw	d0,d0			 | switch on the type field 
	movw	pc@(6,d0:w),d1
	jmp	pc@(2,d1:w)
Ltable:
	.word	Lerr-Ltable			| unknown type
	.word	Lzero-Ltable			| zero
	.word	Lgu-Ltable			| gradual undeflow
	.word	Lplain-Ltable			| ordinary number
	.word	Linf-Ltable			| infinity
	.word	Lnan-Ltable			| Nan
	.word	Lerr-Ltable			| unknown type
	.word	Lerr-Ltable			| unknown type
Lplain:
	movl	d7,d0
	bclr	#31,d0		| d0 gets abs(x).
	cmpl	ONE,d0
	bcs	lt1		| Branch if abs(x) < 1.
	beqs	exactone	| Branch if argument = +-1.
	sf	d7		| d7 gets 0 if abs(x) < 1, else -1.
	movl	d0,d1
	movl	ONE,d0
	jsr	fvdivis		| d0 gets 1/abs(x).
	bras	asquare
exactone:
	movl	d7,d0
	andl	#0x80000000,d0	| d0 gets sign(x).
	orl	PIO4,d0		| do gets +- pi/4.
	jra	Ldone
lt1:
	movl	d7,d0		| d0 gets argument a = x.
	st	d7
asquare:
	movl	d0,d6		| d6 saves approx argument a.
	movl	d0,d1		| d1 gets a.
	jsr 	fvmulis		| d0 gets a**2.
	movl	d0,d5		| d5 saves a**2.
	bsr 	tofix		| d0 gets fixed point a**2.
	tstl	d0
	bnes	doapprox	| Branch if a**2 is not negligible.
	movl	d6,d0		| d0 gets a, the approximate result.
	bras	checkreversal
doapprox:
	movl	d0,d1		| d1 gets a**2.
	movl	d1,d2		| d2 gets a**2.
	swap	d2		| d2 gets swap(a**2) for fixmulu.
|	movl	q4,d0
|	bsrs	fixmulu
	lsrl	#6,d0		| q4 = 1/64 so do it fast.
	addl	Q3,d0
	bsr 	fixmulu
	addl	Q2,d0
	bsr  	fixmulu
	addl	Q1,d0
	bsr 	fromfix
	movl	d0,a0		| a0 saves q = q4*a2+q3)*a2+q2)*a2+q1.
	movl	P4,d0
	bsrs	fixmulu
	addl	P3,d0
	bsrs	fixmulu
	addl	P2,d0		| d0 gets p = p4*a2+p3)*a2+p2.
	bsr 	fromfix
	movl	a0,d1
	jsr	fvdivis		| d0 gets p/q.
	subl	#0x01000000,d0	| d0 gets r = (p/q)/4.
	movl	d5,d1
	jsr	fvmulis		| d0 gets a**2 * r.
	movl	d6,d1
	jsr	fvmulis		| d0 gets a * a**2 * r.
	movl	d0,d1
	movl	d6,d0
	jsr	fvsubis		| d0 gets a - a**3 * r.
checkreversal:
	tstb	d7
	bnes	Ldone		| Branch if abs(x) < 1.0.
	movl	PIO2b,d1
	jsr	fvaddis
	movl	PIO2,d1
	jsr	fvsubis		| d0 gets atan(abs(1/x)) - pi/2.
	tstl	d7
	bmis	Ldone		| Branch if x < 0.
	bchg	#31,d0		| else reverse sign
Ldone:
	moveml sp@+,#RESTALL
	RET
Lzero:
Lgu:
Lerr:
Lnan:
	movl	d7,d0
	bras	Ldone
Linf:
	movl	d7,d0
	andl	#0x7ff00000,d0		| Clear all but sign.
	orl	PIO2,d0			| Make pi/2.
	bras	Ldone

| 	fixmulu does unsigned fix point multiply:
| 	bit 31 is 0.5, bit 30 is 0.25, etc.
|	On input:
|	d0 contains Y
| 	d1 contains X
|	d2 contains X swapped
|	d3 is scratch
|	d4 is scratch
|	On output:
|	d0 contains Y*X
|	d1 and d2 are unchanged
|	d3 and d4 are lost

fixmulu:
	movl	d0,d3	| d3 gets Y.
	swap	d0	| d0 gets Y swapped = Ys.
	movl	d0,d4	| d4 gets Ys.
	mulu	d2,d0	| d0 gets Ys*Xs.
	mulu	d2,d3	| d3 gets Y*Xs.
	clrw	d3
	swap	d3
	addl	d3,d0
	mulu	d1,d4	| d4 gets Ys*X.
	clrw	d4
	swap	d4
	addl	d4,d0
	rts

| 	tofix converts a positive normalized single to unsigned fix point
|	Input and Output are in d0.
|	d3 is scratch.

tofix:
	roll	#1,d0	| Rotate sign.
	roll	#8,d0	| Rotate exponent.
	movb	#126,d3
	subb	d0,d3	| d3 gets right shift count.
	clrb	d0	| Clear exponent field.
	lsrl	#1,d0	| Make room for I bit.
	bset	#31,d0	| Turn on I bit.
	cmpb	#31,d3
	blss	1$	| Branch if reasonable shift.
	clrl	d0	| Large shift so number is zero.
	rts
1$:
	lsrl	d3,d0	| Do shift.
	rts

| 	fromfix converts an unsigned nonzero fix point to 
|	positive normalized single.
|	Input and Output are in d0.
|	d3, d4 are scratch.

fromfix:
	movl	#118*0x800000,d4 | d4 gets exponent as if bit 23 were leading.
	movl	d0,d3		| d3 gets x.
	bras	testhigh
shiftdown:
	lsrl	#1,d0
	lsrl	#1,d3
	addl	#0x800000,d4
testhigh:
	andl	#0xff000000,d3	| d3 gets high order bits of x.
	bnes	shiftdown
	bras	testleading
shiftup:
	lsll	#1,d0
	subl	#0x800000,d4	| Adjust exponent.
testleading:
	btst	#23,d0
	beqs	shiftup		| Branch if leading bit still zero.
	bclr	#23,d0		| Now remove implicit bit.
	orl	d4,d0		| Insert exponent.
	rts

        .data 
        .asciz  "@(#)s_exp.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .text
 
|	Copyright (c) 1983 by Sun Microsystems, Inc.

#include "DEFS.h"

        .even 
        .data
	.globl	fvexpis
fvexpis:        jsr     sky_switch:L
                jmp     Fexpis:L
                jmp     Sexpis:L
        .even
        .text
 
| Single precision exp function from Cody and Waite "Software Manual for
| the Elementary Functions" Prentice Hall, 1980.

	SAVED0D1 = 0x0003
	RESTD0D1 = 0x0003
	SAVEALL	 = 0x3F00	| registers d2-d7
	RESTALL	 = 0x00fc	

one:	.long	0x3f800000	| 1.0
p0:	.long	0x3E800000	| 2.500000e-01
p1:	.long	0x3B885308	| 4.160289e-03
q0:	.long	0x3F000000	| 5.000000e-01
q1:	.long	0x3D4CBF5B	| 4.998718e-02

| temp regs
|		d7 X
| temps (off of a6):
|		1-4	X
|		8-12 double precision X
|		16 N
|		20-24 double precision XN*ln(2)
|		28 g
|		32 z
|		36 g*P(z)

	.globl	Fexpis
Fexpis:
	link	a6,#-36
	RTMCOUNT
	moveml	#SAVEALL,sp@-			| save d2-d7
	movl	d0,a6@(-4)			| save 
	movl	d0,d1
	jsr	f_tst				| test the argument
	addw	d0,d0			 	| switch on the type field 
	movw	pc@(6,d0:w),d2
	jmp	pc@(2,d2:w)
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
	cmpb	#134,d1		| Biased exponent is still in d1 from tst.
	jcc	bigexp		| Avoid argument reduction if result too big
				| or too small.
	movl	a6@(-4),d0			| d0 gets argument.
	jsr	s_exparg	| d0 gets reduced argument x;
				| d1 gets quotient i.
	movl	d0,a6@(-28)	| -28 gets x.
	cmpl	#200,d1
	blts	1$		| Branch if argument not huge.
	movw	#200,a6@(-16)	| Store modified value.
	bras	2$
1$:
	cmpl	#-200,d1
	bgts	3$		| Branch if argument not teeny.
	movw	#-200,a6@(-16)	| Store modified value.
	bras	2$
3$:
	movw	d1,a6@(-16)	| Save exponent, with abs value < 200.
2$:
	movl	d0,d1		| d1 gets x.
	jsr	fvmulis				| z=g**2 -> d0
	movl	d0,a6@(-32)
	movl	p1,d1
	jsr	fvmulis				| p1*z -> d0
	movl	p0,d1
	jsr	fvaddis				| p1*z + p0 -> d0
	movl	a6@(-28),d1
	jsr	fvmulis				| g*P(z) -> d0
	movl	d0,a6@(-36)			| save it 
	movl	a6@(-32),d0			| z -> d0
	movl	q1,d1
	jsr	fvmulis				| q1*z -> d0
	movl	q0,d1
	jsr	fvaddis				| Q(z) -> d0
	movl	a6@(-36),d1			
	jsr 	fvsubis				| Q(z) - g*P(z) -> d0
	movl	d0,d1
	movl	a6@(-36),d0
	jsr	fvdivis				| g*P(z)/(Q(z)-g*P(z)) -> d0
	addl	#0x800000,d0	| Double quotient.
	movl	one,d1
	jsr	fvaddis				|
	movl	d0,d1
	jsr	f_unpk
	addw	a6@(-16),d2
	jsr	f_pack
	movl	d1,d0
Ldone:	moveml	sp@+,#RESTALL
	unlk	a6
	rts
Lzero:
Lgu:
	movl	one,d0		| easy special case
	bras	Ldone
bigexp:
Linf:
	movl	a6@(-4),d0	| exp(+inf) = +inf.
	bpls	plusinf	
	clrl	d0		| exp(-inf) = +0.
	bras	Ldone
plusinf:
	movl	#0x7f800000,d0	| exp(+inf or +big) = +inf.
	bras	Ldone
Lerr:
Lnan:
	movl	a6@(-4),d0	| Return argument.
	bras	Ldone

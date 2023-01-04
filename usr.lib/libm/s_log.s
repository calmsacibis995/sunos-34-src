        .data 
        .asciz  "@(#)s_log.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .text

|	Copyright (c) 1984 by Sun Microsystems, Inc.

#include "DEFS.h"

        .even 
        .data
	.globl	fvlogis
fvlogis:        jsr     sky_switch:L
                jmp     Flogis:L
                jmp     Slogis:L
        .even
        .text
 
| single precison log function from Cody and Waite "Software Manual for
| the Elementary Functions" Prentice Hall, 1980.

	SAVED0D1 = 0x0003
	RESTD0D1 = 0x0003
	SAVEALL	 = 0x3F00	| registers d2-d7
	RESTALL	 = 0x00fc	

C0	=	0x3F3504F3	| 7.071068e-01 < sqrt(0.5)
C1:	.long	0x3F318000	| 6.933594e-01
C2:	.long	0xB95E8083	| -2.121944e-04
A0:	.long	0xBF0D7E3D	| -5.527075e-01
B0:	.long	0xC0D43F3A	| -6.632718e+00
ONE	=	0x3f800000	| 1.0
EXPONE	=	0x00800000	| Exponent of 1 for mult or div by 2.

| temp regs:
|		d7	N=INTXP(X) then XN = FLOAT(N) 
|		d6	f=SETXP(X,0) then znum/zden = 2(f-1)/(f+1)
|		d5	address scratch then znum then w*(A0+w) then R(z)
|		d4	address scratch then w then C2*XN+R(z)
|		d3	f-1
	RTENTRY(Flogis)
	moveml	#SAVEALL,sp@-			| save d2-d7
	movl	d0,d1
	movl	d0,d7			| save 
	jsr	f_tst				| test the argument
	addw	d0,d0			| switch on the type field
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
Lgu:
	movl	d7,d1			| no sky: unpack argument
	jsr	f_unpk
	tstw	d3
	jmi 	Lerrarg				| Branch if negative arg.
	extl	d2
	addl	#24,d2				| INTXP(X) -> d2
	movl	d2,d7				| save it
	movl	#-24,d2				| make f=SETXP(X,0) 
	jsr	f_pack
	cmpl	#C0,d1
	bgts	2$				| if C0 < f skip; else
	subql	#1,d7				| N-1 -> N
	addl	#EXPONE,d1			| Now sqrt(0.5) < f < sqrt(2).
2$:	movl	d1,d6				| and save it
	movl	#ONE,d0
	jsr	fvaddis				| d0 gets f+1.
	exg	d0,d6				| d6 gets f+1, d0 gets f.
	movl	#ONE,d1	
	jsr	fvsubis				| f -1 -> d0
	movl	d0,d3				| d3 saves f-1.
	movl	d6,d1
	jsr	fvdivis				
	tstl	d0
	beqs	3$				| Branch if exact zero.
	addl	#EXPONE,d0			| znum/zden -> d0
3$:	movl	d0,d6				| save z
	movl	d0,d1
	jsr	fvmulis
	movl	d0,d4				| save z**2
	movl	A0,d1
	jsr	fvmulis				| w*A0 -> d0
	movl	d0,d5				| save it
	movl	d4,d0
	movl	B0,d1
	jsr	fvaddis				| B0+w -> d0
	movl	d0,d1
	movl	d5,d0
	jsr	fvdivis				| r(w) -> d0
	movl	d3,d1				| d1 gets f-1.
	beqs	4$				| Branch if exact zero.
	subl	#EXPONE,d1			| d1 gets (f-1)/2.
4$:	jsr	fvsubis				| d0 gets r(w)-(f-1)/2.
	movl	d6,d1
	jsr	fvmulis				| z*r(w) -> d0
	movl	d3,d1
	jsr	fvaddis				| f + z*r(w) -> d0
	movl	d0,d5				| save R(z)
	movl	d7,d1
	jsr	i_unpk
	jsr	f_pack				| have XN in d1
	movl	d1,d7				| save it
	movl	C2,d0
	jsr	fvmulis				| C2*XN -> d0
	movl	d5,d1
	jsr	fvaddis				| C2*XN+R(z) -> d0
	movl	d0,d4				| save it
	movl	d7,d0
	movl	C1,d1
	jsr	fvmulis				| C1*XN -> d0
	movl	d4,d1
	jsr	fvaddis				| (XN*C2+R(z))+XN*C1 -> d0
Ldone:
	moveml sp@+,#RESTALL
	RET	
	

Lzero:
	movl	#0xFF800000,d0		| log(+-0) = -inf.
	bras	Ldone
Lnan:
	movl	d7,d0		| log(nan) is nan.
	bras	Ldone
Linf:
	movl	d7,d0
	bpls	Ldone			| log(+inf) = +inf.
Lerr:
Lerrarg:
	jsr	f_snan
	bras	Ldone


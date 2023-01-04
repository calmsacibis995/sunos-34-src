        .data 
        .asciz  "@(#)Fsins.s 1.1 86/09/25 Copyr 1985 Sun Micro"
        .even
        .text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

| single precison sin function from Cody and Waite "Software Manual for
| the Elementary Functions" Prentice Hall, 1980.

	SAVED0D1 = 0x0003
	RESTD0D1 = 0x0003
	SAVEALL	 = 0x3F00	| registers d2-d7
	RESTALL	 = 0x00fc	

R1:	.long	0xBE2AAAA4	| -1.666666e-01
R2:	.long	0x3C08873E	| 8.333026e-03
R3:	.long	0xB94FB222	| -1.980742e-04
R4:	.long	0x362E9C5B	| 2.601903e-06

			| Cosine approximations.
ONE:
p0:	.long	0x3F800000	| 1.000000e+00
p1:	.long	0xBF000000	| -0.5
p2:	.long	0x3D2AAA9C
p3:	.long	0xBAB60376
p4:	.long	0x37CC73EE

| temp regs:
|			d4 holds f
|			d5 holds g
|			d7 holds y

RTENTRY(Fsins)
	moveml	#SAVEALL,sp@-			| save d2-d7
	movl	d0,d1
	movl	d1,d7			| save 
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
	jsr	s_trigarg	
	bsrs	sincos
Ldone:
	moveml sp@+,#RESTALL
	RET	
Lzero:
Lgu:
Lnan:
	movl	d7,d0
	bras	Ldone
Lerr:
Linf:
	jsr	f_snan
	bras	Ldone

sincos:			| Reduced argument x is in d0, quotient in d1.
			| Sin(x) is returned in d0.
			| d4 and d5 are used as to hold x and x*x.
	movb	d1,d7	| d7 gets the two bits of quotient that matter.
	movl	d0,d1
	movl	d0,d4	| d4 saves x.
	jsr	fvmulis		| d0 gets x**2.
	movl	d0,d5	| d5 gets x**2.
	btst	#0,d7
	beqs	dosin	| In even quadrants compute sin, odd cos.
	movl	p4,d1
	jsr	fvmulis
	movl	p3,d1
	jsr	fvaddis
	movl	d5,d1
	jsr	fvmulis
	movl	p2,d1
	jsr	fvaddis
	movl	d5,d1
	jsr	fvmulis
	movl	p1,d1
	jsr	fvaddis
	movl	d5,d1
	jsr	fvmulis
	movl	p0,d1
	jsr	fvaddis		| d0 gets p0+x2*(p1+x2*(p2+x2*(p3+x2*p4.
	bras	scsign
dosin:
	movl	R4,d1			| compute the polynomial approximation
	jsr	fvmulis
	movl	R3,d1				
	jsr	fvaddis
	movl	d5,d1
	jsr	fvmulis
	movl	R2,d1
	jsr	fvaddis
	movl	d5,d1
	jsr	fvmulis
	movl	R1,d1
	jsr	fvaddis
	movl	d5,d1
	jsr	fvmulis
	movl	d4,d1				| f*p(g) -> d0
	jsr	fvmulis
	movl	d4,d1				| f+d0	-> d0
	jsr	fvaddis
scsign:
	btst	#1,d7
	beqs	1$		| Reverse sign in negative quadrants.
	bchg	#31,d0
1$:
	rts

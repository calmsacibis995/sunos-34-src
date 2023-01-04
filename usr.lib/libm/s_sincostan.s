        .data 
        .asciz  "@(#)s_sincostan.s 1.1 86/09/25 Copyr 1984 Sun Micro"
        .even
        .text

|	Copyright (c) 1984 by Sun Microsystems, Inc.

#include "DEFS.h"

        .even 
        .data
	.globl	fvsinis,fvcosis,fvtanis
fvsinis:        jsr     sky_switch:L
                jmp     Fsinis:L
                jmp     Ssinis:L
fvcosis:        jsr     sky_switch:L
                jmp     Fcosis:L
                jmp     Scosis:L
fvtanis:        jsr     sky_switch:L
                jmp     Ftanis:L
                jmp     Stanis:L
        .even
        .text
 
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

			| Tangent approximation.
tp1: 	.long   0xBDC433B8
tq1:	.long   0xBEDBB7AF
tq2:	.long   0x3C1F3375

| temp regs:
|			d4 holds f
|			d5 holds g
|			d7 holds y
	RTENTRY(Fsinis)
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

Lcosplain:
        movl    d7,d0
        jsr     s_trigarg
        addqw	#1,d1			| Offset Q for cosine.
	bsrs    sincos
	bras	Ldone
Lcoszero:
Lcosgu:
        movl    ONE,d0                          | easy special case
        bras    Ldone

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
	
	RTENTRY(Fcosis)
        moveml  #SAVEALL,sp@-                   | save d2-d7
        movl    d0,d1
        movl    d1,d7                      | save
        jsr     f_tst                           | test the argument
        addw    d0,d0                   | switch on the type field
        movw    pc@(6,d0:w),d1
        jmp     pc@(2,d1:w)
Ctable:
        .word   Lerr-Ctable                     | unknown type
        .word   Lcoszero-Ctable                    | zero
        .word   Lcosgu-Ctable                      | gradual undeflow
        .word   Lcosplain-Ctable                   | ordinary number
        .word   Linf-Ctable                     | infinity
        .word   Lnan-Ctable                     | Nan
        .word   Lerr-Ctable                     | unknown type
        .word   Lerr-Ctable                     | unknown type
	
	RTENTRY(Ftanis)
        moveml  #SAVEALL,sp@-                   | save d2-d7
        movl    d0,d1
        movl    d1,d7                      | save
        jsr     f_tst                           | test the argument
        addw    d0,d0                   | switch on the type field
        movw    pc@(6,d0:w),d1
        jmp     pc@(2,d1:w)
Ttable:
        .word   Lerr-Ttable                     | unknown type
        .word   Lzero-Ttable                    | zero
        .word   Lgu-Ttable                      | gradual undeflow
        .word   Ltanplain-Ttable                   | ordinary number
        .word   Linf-Ttable                     | infinity
        .word   Lnan-Ttable                     | Nan
        .word   Lerr-Ttable                     | unknown type
        .word   Lerr-Ttable                     | unknown type

Ltanplain:
	movl	d7,d0			
	jsr	s_trigarg	
	bsrs	tanx
	jra	Ldone

tanx: 			| Reduced argument x is in d0, quotient in d1.
			| Tan(x) is returned in d0.
			| d4 and d5 are used as to hold x and x*x.
			| d6 is used to hold q.
	movb	d1,d7	| d7 gets the two bits of quotient that matter.
	movl	d0,d1
	movl	d0,d4		| d4 saves x.
	jsr	fvmulis		| d0 gets x**2.
	movl	d0,d5		| d5 gets x**2.
	movl	tq2,d1		
	jsr	fvmulis
	movl	tq1,d1				
	jsr	fvaddis
	movl	d0,d6		| d6 saves q = q2*x2 + q1.
	movl	d0,d1
	movl	tp1,d0
	jsr	fvsubis
	exg	d0,d6		| d6 gets p-q, d0 gets q.
	movl	d5,d1
	jsr	fvmulis
	movl	ONE,d1
	jsr	fvaddis
	movl	d0,d1		| d1 gets 1+x2*q.
	movl	d6,d0
	jsr	fvdivis		| d0 gets (p-q)/(1+x2*q).
	movl	d5,d1
	jsr	fvmulis		| d0 gets x2*
	movl	d4,d1			
	jsr	fvmulis		| d1 gets x*x2*
	movl	d4,d1				| f+d0	-> d0
	jsr	fvaddis
	btst	#0,d7
	beqs	1$		
	movl	d0,d1		| d1 gets tan(x).
	movl	ONE,d0
	bchg	#31,d0		| d0 gets -1.
	jsr	fvdivis		| In odd quadrants return -1/tan.
1$:
	rts
	

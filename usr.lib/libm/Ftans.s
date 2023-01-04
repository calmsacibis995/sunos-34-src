        .data 
        .asciz  "@(#)Ftans.s 1.1 86/09/25 Copyr 1985 Sun Micro"
        .even
        .text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

| single precison tan function from Cody and Waite "Software Manual for
| the Elementary Functions" Prentice Hall, 1980.

	SAVED0D1 = 0x0003
	RESTD0D1 = 0x0003
	SAVEALL	 = 0x3F00	| registers d2-d7
	RESTALL	 = 0x00fc	

ONE:	.single	0r1.0

				| Tangent approximation.
tp1: 	.long   0xBDC433B8
tq1:	.long   0xBEDBB7AF
tq2:	.long   0x3C1F3375

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

RTENTRY(Ftans)
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
	

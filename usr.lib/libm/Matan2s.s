        .data
        .asciz  "@(#)Matan2s.s 1.1 86/09/25 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

/*
	atan2(y,x) = atan(y/x)

	y is in d0
	x is in d1

*/

RTENTRY(Matan2s)
	movel	d0,sp@-		| Save y.
	fmoves	d0,fp0		| fp0 gets y.
	fdivs	d1,fp0		| fp0 gets y/x.
        fmovel  fpsr,d0          | d0 gets status after divide.
        btst    #13,d0          | Operand error set by 0/0 or inf/inf.
        beqs    3f              | Branch if no 0/0 or inf/inf.
        fmovecrx #0x0,fp0       | fp0 gets pi.
        fscalew	#-2,fp0		| fp0 gets pi/4.
	movel	sp@,d0
	eorl	d1,d0
        bpls    4f              | Branch if quotient was not negative.
        fnegx   fp0,fp0         | fp0 gets -pi/4.
	bras	4f
3:
	fatanx	fp0,fp0		| fp0 gets atan(y/x).
4:
	tstl	d1	
	bpls	Matan2send	| Branch if x >= 0.
	fmovecr	#0,fp1		| fp1 gets pi.
	tstb	sp@
	bmis	1f		| Branch if y < 0.
	faddx	fp1,fp0 	| fp0 gets atan(y/x)+pi.
	bras	2f
1:	
	fsubx	fp1,fp0 	| fp0 gets atan(y/x)-pi.
2:	
Matan2send:
	fmoves	fp0,d0		| Save result.
	addql	#4,sp		| Bypass y.
	RET

        .data
        .asciz  "@(#)Watan2s.s 1.1 86/09/25 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Wdefs.h"

/*
	atan2(y,x) = atan(y/x)

	y is in d0
	x is in d1

*/

RTENTRY(Watan2s)
	movel	d0,sp@-			| Save y.
	LOADFPABASE
	fpmoves@1	d0,fpa0		| fp0 gets y.
	fpmcmps@1	d1,fpa0
	fpmove@1	fpastatus,d0
	movew		d0,cc
	jfne		3f		| Branch if magnitudes differ.
	movel		sp@,d0
	eorl		d1,d0
	bmis		4f		| Branch if signs differ.
	fpmoves@1	#0r7.853981633974483096E-1,fpa0		| pi/4.
	bras		5f
4:
	fpmoves@1	#0r-7.853981633974483096E-1,fpa0	| -pi/4.
	bras		5f
3:
	fpdivs@1	d1,fpa0		| fp0 gets y/x.
	fpatans@1	fpa0,fpa0	| fp0 gets atan(y/x).
5:
	tstl	d1	
	bpls	Watan2send		| Branch if x >= 0.
	fpmoves@1	#0r3.141592653589793239,fpa1		| fp1 gets pi.
	tstb	sp@
	bmis	1f			| Branch if y < 0.
	fpadds@1	fpa1,fpa0 	| fp0 gets atan(y/x)+pi.
	bras	2f
1:	
	fpsubs@1	fpa1,fpa0 	| fp0 gets atan(y/x)-pi.
2:	
Watan2send:
	fpmoves@1	fpa0,d0		| Save result.
	addql	#4,sp			| Bypass y.
	RET

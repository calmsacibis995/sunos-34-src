        .data
        .asciz  "@(#)Watan2d.s 1.1 86/09/25 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Wdefs.h"

/*
	atan2(y,x) = atan(y/x)

	y is in d0/d1
	x is in a0@

*/

RTENTRY(Watan2d)
	LOADFPABASE
	fpmoved@1	d0:d1,fpa0	| fpa0 gets y.
	fpmcmpd@1	a0@,fpa0
	fpmove@1	fpastatus,d1
	movew		d1,cc
	jfne		3f		| Branch if magnitudes unequal.
	movel		a0@,d1
	eorl		d0,d1
	bmis		4f		| Branch if signs differ.
	fpmoved@1	#0r7.853981633974483096E-1,fpa0 	| pi/4
	bras		5f
4:	
	fpmoved@1	#0r-7.853981633974483096E-1,fpa0	| -pi/4
	bras		5f
3:
	fpdivd@1	a0@,fpa0	| fpa0 gets y/x.
	fpatand@1	fpa0,fpa0	| fpa0 gets atan(y/x).
5:
	tstb		a0@	
	bpls		Watan2dend	| Branch if x >= 0.
	fpmoved@1	#0r3.141592653589793239,fpa1		| pi
	tstl		d0
	bmis		1f		| Branch if y < 0.
	fpaddd@1	fpa1,fpa0 	| fpa0 gets atan(y/x)+pi.
	bras		2f
1:	
	fpsubd@1	fpa1,fpa0 	| fpa0 gets atan(y/x)-pi.
2:	
Watan2dend:
	fpmoved@1	fpa0,d0:d1	| Save result.
	RET

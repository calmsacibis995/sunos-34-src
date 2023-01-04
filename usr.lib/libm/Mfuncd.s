        .data
        .asciz  "@(#)Mfuncd.s 1.1 86/09/25 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

#define DMONADIC(g) \
RTENTRY(M/**/g/**/d) ; \
	moveml	d0/d1,sp@- ; \
	f/**/g/**/d	sp@,fp0 ; \
	fmoved	fp0,sp@ ; \
	moveml	sp@+,d0/d1 ; \
	RET

#define DMONADICX(g,h) \
RTENTRY(M/**/g/**/d) ; \
	moveml	d0/d1,sp@- ; \
	f/**/h/**/d	sp@,fp0 ; \
	fmoved	fp0,sp@ ; \
	moveml	sp@+,d0/d1 ; \
	RET

DMONADICX(exp,etox)
DMONADICX(exp1,etoxm1)
DMONADICX(log1,lognp1)
DMONADICX(pow2,twotox)
DMONADICX(pow10,tentox)
DMONADIC(sinh)
DMONADIC(cosh)
DMONADIC(tanh)
DMONADIC(sin)
DMONADIC(cos)
DMONADIC(tan)
DMONADIC(asin)
DMONADIC(acos)
DMONADIC(atan)
	
|DMONADICX(log,logn)
RTENTRY(Mlogd)
	moveml	d0/d1,sp@-
	fmoved	sp@,fp0
	fcmps	#0r0.5,fp0
	fjule	1f		| Branch if x <= 0.5.
	fsubl	#1,fp0
	flognp1x fp0,fp0	| This is more accurate for x > 0.5.
	bras	2f
1:
	flognx	fp0,fp0		| This is more accurate for x < 0.5.
2:
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET
|DMONADIC(log2)
RTENTRY(Mlog2d)
	moveml	d0/d1,sp@-
	fmoved	sp@,fp0
	fcmps	#0r0.5,fp0
	fjule	1f		| Branch if x <= 0.5.
	fsubl	#1,fp0
	flognp1x fp0,fp0	| This is more accurate for x > 0.5.
	fmovecrx #0xd,fp1	| fp1 gets log2(e)
	fmulx	fp1,fp0		| fp0 gets log2(x)
	bras	2f
1:
	flog2x	fp0,fp0		| This is more accurate for x < 0.5.
2:
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET
|DMONADIC(log10)
RTENTRY(Mlog10d)
	moveml	d0/d1,sp@-
	fmoved	sp@,fp0
	fcmps	#0r0.5,fp0
	fjule	1f		| Branch if x <= 0.5.
	fsubl	#1,fp0
	flognp1x fp0,fp0	| This is more accurate for x > 0.5.
	fmovecrx #0xe,fp1	| fp1 gets log10(e)
	fmulx	fp1,fp0		| fp0 gets log10(x)
	bras	2f
1:
	flog10x	fp0,fp0		| This is more accurate for x < 0.5.
2:
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET

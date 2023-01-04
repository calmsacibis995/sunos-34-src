        .data
        .asciz  "@(#)Mfuncs.s 1.1 86/09/25 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

#define SMONADIC(g) \
RTENTRY(M/**/g/**/s) ; \
	f/**/g/**/s	d0,fp0 ; \
	fmoves	fp0,d0 ; \
	RET	

#define SMONADICX(g,h) \
RTENTRY(M/**/g/**/s) ; \
	f/**/h/**/s	d0,fp0 ; \
	fmoves	fp0,d0 ; \
	RET	

SMONADICX(exp,etox)
SMONADICX(exp1,etoxm1)
SMONADICX(pow2,twotox)
SMONADICX(pow10,tentox)
SMONADICX(log,logn)
SMONADICX(log1,lognp1)
SMONADIC(log2)
SMONADIC(log10)
SMONADIC(sinh)
SMONADIC(cosh)
SMONADIC(tanh)
SMONADIC(sin)
SMONADIC(cos)
SMONADIC(tan)
SMONADIC(asin)
SMONADIC(acos)
SMONADIC(atan)

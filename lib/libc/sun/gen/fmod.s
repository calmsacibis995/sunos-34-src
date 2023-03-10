|	.data
|       .asciz  "@(#)fmod.s 1.1 86/09/24 Copyr 1986 Sun Micro"
	.even
	.text

|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

/*
 * double
 * fmod ( x, y )
 *	double x, y ;
 *
 * return a value v such that (x-v)/y is an integral value
 * and sign(v) = sign(x), which is equivalent to Fortran MOD.
 */

ENTRY(fmod)
	moveml	PARAM,d0/d1	| d0/d1 gets x.
	lea	PARAM3,a0	| a0 gets address of y.
	jsr	Vmodd
	RET

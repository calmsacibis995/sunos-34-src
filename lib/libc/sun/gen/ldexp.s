       .data
|       .asciz  "@(#)ldexp.s 1.1 86/09/24 Copyr 1986 Sun Micro"
       .even
       .text

|       Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "sys/errno.h"

/*
 * double
 * ldexp( value, exp)
 *      double value;
 *      int exp;
 *
 * return a value v s.t. v == value * (2**exp)
 */

ENTRY(ldexp)
	moveml	PARAM,d0/d1	| suck parameters into registers
	lea	PARAM3,a0	| a0 gets address of i.
	jsr	Vscaleid
	movel	d2,a0		| a0 stores d2.
	movel	d0,d2
	andl	#0x7ff00000,d2	| d2 gets exp(result).
	beqs	2f		| Branch if zero or subnormal result.
	cmpl	#0x7ff00000,d2	
	beqs	3f		| Branch if inf or nan result.
	bras	4f
2:				| Zero or subnormal result.
|		Result was zero/subnormal; therefore arg was zero/finite.
	cmpl	PARAM,d0
	bnes	5f		| Branch if arg <> result.
	cmpl	PARAM2,d1
	bnes	5f		| Branch if arg <> result.
	bras	4f
3:				| Inf or NaN result.
	tstl	d1
	bnes	4f		| Return if NaN.
	movel	d0,d2
	andl	#0x000fffff,d2	| Abs(result).
	bnes	4f		| Return if NaN.
|		Result was inf; therefore arg was finite or inf = result.
	cmpl	PARAM,d0
	beqs	4f		| Branch if arg = inf.
5:	| Underflow or overflow.
	movel	#ERANGE,_errno
4:
	movel	a0,d2		| Restore d2.
	RET	

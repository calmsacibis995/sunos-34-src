	.data
	.asciz	"@(#)r_sign.s 1.1 86/09/25 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(_r_sign)
	movel	PARAM,a0
	movel	a0@,d0
	movel	PARAM2,a0
	cmpl	#0x80000000,a0@
	blss	1f		| Branch if y >= +0.
3:
	bset	#31,d0
	bras	2f
1:
	bclr	#31,d0
2:
	RET

	.data
	.asciz	"@(#)pow_ri.s 1.1 86/09/25 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(_pow_ri)
	movel	PARAM,a0
	movel	a0@,d0
	movel	PARAM2,a0
	movel	a0@,d1
#ifdef PROF
	jsr	Vpowis
	RET
#else
	jmp	Vpowis
#endif

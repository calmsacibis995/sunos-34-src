	.data
	.asciz	"@(#)r_atan.s 1.1 86/09/25 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "DEFS.h"

RTENTRY(_r_atan)
	movel	PARAM,a0
	movel	a0@,d0
#ifdef PROF
	jsr	Vatans
	RET
#else
	jmp	Vatans
#endif

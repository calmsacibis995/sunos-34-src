	.data
	.asciz	"@(#)i_dnnt.s 1.1 86/09/25 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(_i_dnnt)
	movel	PARAM,a0
	moveml	a0@,d0/d1
#ifdef PROF
	jsr	Vnintd
	RET
#else
	jmp	Vnintd
#endif

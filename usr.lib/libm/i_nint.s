	.data
	.asciz	"@(#)i_nint.s 1.1 86/09/25 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(_i_nint)
	movel	PARAM,a0
	movel	a0@,d0
#ifdef PROF
	jsr	Vnints
	RET
#else
	jmp	Vnints
#endif

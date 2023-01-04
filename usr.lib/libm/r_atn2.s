	.data
	.asciz	"@(#)r_atn2.s 1.1 86/09/25 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(_r_atn2)
	movel	PARAM,a0
	movel	a0@,d0
	movel	PARAM2,a0
	movel	a0@,d1
#ifdef PROF
	jsr	Vatan2s
	RET
#else
	jmp	Vatan2s
#endif

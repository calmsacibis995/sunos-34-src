	.data
	.asciz	"@(#)FMscaled.s 1.1 86/09/25 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(_FMscaled_)
	movel	PARAM,a0
	moveml	a0@,d0/d1
	movel	PARAM2,a0
#ifdef PROF
	jsr	Mscaleid
	RET
#else
	jmp	Mscaleid
#endif

RTENTRY(_FMexpod_)
	movel	PARAM,a0
	moveml	a0@,d0/d1
#ifdef PROF
	jsr	Fexpod
	RET
#else
	jmp	Fexpod
#endif

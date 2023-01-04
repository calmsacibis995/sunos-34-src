        .data
|       .asciz  "@(#)Fatans.s 1.1 86/09/25 Copyr 1986 Sun Micro"
        .even
        .text

|       Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(Fatans)
	movel	d0,sp@-		| Move argument to stack.
	jsr	_CFatans	| Compute atan.
	addql	#4,sp		| Remove argument.
	RET

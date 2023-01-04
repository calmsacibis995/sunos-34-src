        .data
        .asciz  "@(#)Vmode.s 1.1 86/09/24 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

VECTORED(mode)

	.text

ENTER(_fpmode_)
	movel	sp@(4),a0	| Get address of long argument.
	movel	a0@,d0		| Get long argument.
	jra	Vmode

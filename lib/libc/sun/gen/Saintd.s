	.data
	.asciz	"@(#)Saintd.s 1.1 86/09/24 Copyr 1985 Sun Micro"
	.even
	.text


|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

#define DUMMY(f) .globl S/**/f ; S/**/f: jmp F/**/f

				| Things the Sky board can't do better than software.
	DUMMY(aintd)
	DUMMY(anintd)
	DUMMY(arintd)
	DUMMY(floord)
	DUMMY(ceild)

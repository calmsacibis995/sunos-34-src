	.data
	.asciz	"@(#)Saints.s 1.1 86/09/24 Copyr 1985 Sun Micro"
	.even
	.text


|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Sdefs.h"

#define DUMMY(f) .globl S/**/f ; S/**/f: jmp F/**/f

				| Things the Sky board can't do better than software.
	DUMMY(aints)
	DUMMY(anints)
	DUMMY(arints)
	DUMMY(floors)
	DUMMY(ceils)


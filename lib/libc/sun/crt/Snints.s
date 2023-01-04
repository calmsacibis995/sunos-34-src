	.data
	.asciz	"@(#)Snints.s 1.1 86/09/24 Copyr 1985 Sun Micro"
	.even
	.text


|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

ENTER(Snints)
	jmp	Fnints
ENTER(Srints)
	jmp	Frints

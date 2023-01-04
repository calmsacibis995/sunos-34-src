        .data
 	.data
|	.asciz	"@(#)Cfinite.s 1.1 86/09/25 Copyr 1986 Sun Micro"
 	.even
 	.text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

	ENTRY(finite)
	movew	PARAM,d0
	andw	#0x7ff0,d0
	cmpw	#0x7ff0,d0
	beqs	1f		| Branch if inf or nan.
	moveq	#1,d0		| Finite.
	bras	2f
1:
	clrl	d0		| inf or nan.
2:
	RET

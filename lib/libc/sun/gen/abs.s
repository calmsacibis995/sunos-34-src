	.data
/*	.asciz	"@(#)abs.s 1.1 86/09/24 SMI"	*/
	.text

#include "DEFS.h"

ENTRY(abs)
	movl	PARAM,d0
	bges	1$
	negl	d0
1$:
	RET

	.data
/*	.asciz	"@(#)remque.s 1.1 86/09/24 SMI"	*/
	.text

#include "DEFS.h"

ENTRY(remque)
	movl	PARAM,a0
	movl	a0@,a1
	movl	a0@(4),a0
	movl	a1,a0@
	movl	a0,a1@(4)
	RET

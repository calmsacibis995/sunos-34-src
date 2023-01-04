	.data
	.asciz	"@(#)Ssqrts.s 1.1 86/09/24 Copyr 1985 Sun Micro"
	.even
	.text


|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Sdefs.h"

RTENTRY(Ssqrts)
	movl	__skybase,SKYBASE 
	movw	#S_SSQRT,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	IORDY
	movl	SKYBASE@,d0
	RET

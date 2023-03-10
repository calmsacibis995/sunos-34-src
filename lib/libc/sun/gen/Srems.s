	.data
	.asciz	"@(#)Srems.s 1.1 86/09/24 Copyr 1985 Sun Micro"
	.even
	.text


|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Sdefs.h"

RTENTRY(Smods)
	movl	__skybase,SKYBASE 
	movw	#S_SMOD3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	d0,sp@-			| Stack gets x.
	IORDY
	movl	SKYBASE@,d0
	eorl	d0,sp@+
	bpls	4f			| Branch if sign(mod)=sign(x).
	movel	d0,sp@-
	eorl	d1,sp@+
	bpls	2f			| Branch if sign(mod) = sign(y)
	movw    #S_SADD3,SKYBASE@(-OPERAND)
        bras	3f
2:
	movw	#S_SSUB3,SKYBASE@(-OPERAND)
3:
	movl	d0,SKYBASE@		| Add or subtract to get sign.
	movl	d1,SKYBASE@
	IORDY
	movl	SKYBASE@,d0
4:
	RET

ENTER(Srems)
	jmp	Frems

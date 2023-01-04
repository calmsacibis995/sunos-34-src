	.data
	.asciz	"@(#)Vswitch.s 1.1 86/09/24 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

ENTER(Vswitch)
RTENTRY(_vswitchfp_)
	moveml	a0/a1/d0,sp@-
	jsr	Vinit
	jsr	float_switch
	moveml	sp@+,a0/a1/d0
	RET

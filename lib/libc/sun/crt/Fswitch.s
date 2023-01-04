	.data
	.asciz	"@(#)Fswitch.s 1.1 86/09/24 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

ENTER(Fswitch)
RTENTRY(_fswitchfp_)
	moveml	a0/a1/d0,sp@-
	jsr	Finit
	jsr	float_switch
	moveml	sp@+,a0/a1/d0
	RET

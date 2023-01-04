	.data
	.asciz	"@(#)Flibm.s 1.1 86/09/25 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(_CFsqrtd)
	moveml	PARAM,d0/d1
	jsr	Fsqrtd
	RET


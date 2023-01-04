	.data
	.asciz	"@(#)Wswitch.s 1.1 86/09/24 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

ENTER(Wswitch)
RTENTRY(_wswitchfp_)
	moveml	a0/a1/d0,sp@-
	jsr	Winit
	tstw	d0
	beqs	1f
	jsr	float_switch
	moveml	sp@+,a0/a1/d0
	RET
1:
	pea	end-begin
	pea	begin
	pea	2
	jsr	_write
	pea	99
	jsr 	__exit
begin:
	.asciz	"SUN FPA floating point not available -- program requires it\012"
end:

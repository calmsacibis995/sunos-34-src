	.data
|	.asciz	"@(#)Sremd.s 1.1 86/09/24 Copyr 1986 Sun Micro"
	.even
	.text


|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Sdefs.h"

RTENTRY(Smodd)
	moveml	d0/d1/d2/a0,sp@-	| Save arguments and scratch.
	jsr	Fremd
	movel	sp@,d2			| d2 gets sign of x.
	eorl	d0,d2			| d2 gets sign of x eor sign of remainder r.
	bpls	ok			| Branch if same signs.
	movel	sp@(12),a0		| a0 gets address of y.
	movel	a0@,d2			| d2 gets sign of y.
	eorl	d0,d2			| d2 gets sign of y eor sign of r.
	bpls	sub			| Branch if same signs, implying subtract.
	jsr	Saddd			| d0/d1 := r + y.
	bras	ok
sub:
	jsr	Ssubd			| d0/d1 := r - y.
ok:
	addql	#8,sp			| Skip old d0/d1.
	moveml	sp@+,d2/a0		| Restore d2/a0.	
	RET

ENTER(Sremd)
	jmp	Fremd

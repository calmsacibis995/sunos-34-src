        .data
|       .asciz  "@(#)Mdefault.s 1.1 86/09/24 Copyr 1986 Sun Micro"
        .even
        .text

|       Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(_Mdefault)                       | Initializes 68881 for default modes.
        fmoveml twozeros,fpcr/fpsr
        RET
twozeros: .long	0,0

|	MA93N()  returns 1 if 68881 is an A93N, 0 if not
|	tests loge(1+eps) and denorm/norm boundary

RTENTRY(_MA93N)
	flognd	onepluseps,fp0		| fp0 = loge(1+2**-52) = 2**-52 approx
	fmoves	fp0,d0
	cmpl	#0x25800000,d0
	bnes	2f
	fmoved	justdenorm,fp0
	fmoves	fp0,d0
	cmpl	#0x00800000,d0
	bnes	2f
	moveq	#1,d0
	bras	1f
2:
	clrl	d0
1:
	RET

onepluseps: .long 0x3FF00000,0x00000001	| 1+2^-52
justdenorm: .long 0x380FFFFF,0xFE000000  | Should round to least single normal.

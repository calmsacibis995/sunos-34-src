        .data
 	.data
|	.asciz	"@(#)Clogb.s 1.1 86/09/25 Copyr 1986 Sun Micro"
 	.even
 	.text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

	ENTRY(logb)
       movel   PARAM,d0
       jsr     Fexpod
       cmpw    #-0x3ff,d0
       beqs    3f
       cmpw    #0x400,d0
       beqs    2f
1:
       jsr     Vfltd
       bras	6f
2:
       moveml  PARAM,d0/d1
       bclr    #31,d0
       lea     dzero,a0
       jsr     Vaddd                   | Add zero to catch signalling NaN.
       bras	6f
3:
       moveml  PARAM,d0/d1
       bclr    #31,d0
       tstl    d0
       bnes    5f
       tstl    d1
       beqs    4f
5:
       movel   #-0x3ff,d0
       bras    1b
4:
       moveml  dmone,d0/d1
       lea     dzero,a0
       jsr     Vdivd           | Set up -1/0 to generate divide by zero signal.
6:
       RET

dzero: .double 0r0
dmone: .double 0r-1

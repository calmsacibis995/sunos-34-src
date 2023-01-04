        .data 
        .asciz  "@(#)Flength2s.s 1.1 86/09/24 Copyr 1985 Sun Micro"
        .even
        .text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(Flength2s)
        moveml  d0/d1/d2/d3,sp@-        | Save x/y.
        jsr     Fexpos                  | d0 gets exponent of x.
        exg	d0,d1			| d0 gets y; d1 gets expo(x).
        jsr     Fexpos                  | d0 gets exponent of y.
        cmpl    d0,d1
        bges    1f
        movel   d0,d2     		| d2 gets scale factor.
        bras    2f
1:

        movel   d1,d2     		| d2 gets scale factor.
2:
        negl    d2        		| Reverse sign.
        movel   sp@+,d0               	| d0 get x.
        movel   d2,d1     		| d1 gets scale factor.
        jsr     Fscaleis                | d0 get x/s.
        jsr     Fsqrs                   | d0 get (x/s)**2
        movel   d0,d3
        movel   sp@+,d0                 | Restore y.
        movel   d2,d1     		| d1 gets address of scale factor.
        jsr     Fscaleis                | d0 get y/s.
        jsr     Fsqrs                   | d0 get (y/s)**2
        movel   d3,d1
        jsr     Fadds                   | d0 get sum of squares.
        jsr     Fsqrts
        movel	d2,d1
	negl	d1
	jsr     Fscaleis
 	moveml	sp@+,d2/d3
	RET

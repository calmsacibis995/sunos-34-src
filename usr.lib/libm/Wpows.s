        .data
        .asciz  "@(#)Wpows.s 1.1 86/09/25 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Wdefs.h"

RTENTRY(Wpows)
|        LOADFPABASE
|	fpstod@1	d1,fpa0		| fp0 gets y.
|	fpmoved@1	fpa0,sp@-	| Stack dble(y).
|        fpstod@1	d0,fpa0		| fp0 gets x.
|	fpmoved@1	fpa0,sp@-	| Stack dble(x).
|        jsr     _CWpows          | d0/d1 gets pow(x,y).
|        addl    #16,sp          | Remove arguments.
|	fpdtos@1	d0:d1,fpa0	| fp0 gets result.
|	fpmoves@1	fpa0,d0
        RET

        .data
        .asciz  "@(#)Sfuncs.s 1.1 86/09/25 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Sdefs.h"

RTENTRY(Sexps)
        movl    __skybase,SKYBASE
        movw    #S_SEXP,SKYBASE@(-OPERAND)
        movl    d0,SKYBASE@
        IORDY
        movl    SKYBASE@,d0
        RET
RTENTRY(Slogs)
        movl    __skybase,SKYBASE
        movw    #S_SLOG,SKYBASE@(-OPERAND)
        movl    d0,SKYBASE@
        IORDY
        movl    SKYBASE@,d0
        RET
RTENTRY(Ssins)
        movl    __skybase,SKYBASE
        movw    #S_SSIN,SKYBASE@(-OPERAND)
        movl    d0,SKYBASE@
        IORDY
        movl    SKYBASE@,d0
        RET
RTENTRY(Scoss)
        movl    __skybase,SKYBASE
        movw    #S_SCOS,SKYBASE@(-OPERAND)
        movl    d0,SKYBASE@
        IORDY
        movl    SKYBASE@,d0
        RET
RTENTRY(Stans)
        movl    __skybase,SKYBASE
        movw    #S_STAN,SKYBASE@(-OPERAND)
        movl    d0,SKYBASE@
        IORDY
        movl    SKYBASE@,d0
        RET
RTENTRY(Satans)
        movl    __skybase,SKYBASE
        movw    #S_SATAN,SKYBASE@(-OPERAND)
        movl    d0,SKYBASE@
        IORDY
        movl    SKYBASE@,d0
        RET
RTENTRY(Spows)
        movl    __skybase,SKYBASE
        movw    #S_SPOW,SKYBASE@(-OPERAND)
        movl    d0,SKYBASE@
        IORDY
        movl    d1,SKYBASE@
        IORDY
        movl    SKYBASE@,d0
        RET


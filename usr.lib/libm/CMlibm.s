	.data
	.asciz	"@(#)CMlibm.s 1.1 86/09/25 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(_CMsqrtd)
	fsqrtd	PARAM,fp0
	fmoved	fp0,sp@-
	moveml	sp@+,d0/d1
	RET

RTENTRY(_CMexpd)
	fetoxd	PARAM,fp0
	fmoved	fp0,sp@-
	moveml	sp@+,d0/d1
	RET

RTENTRY(_CMlogd)
        fmoved  PARAM,fp0
        fcmps   #0r0.5,fp0
        fjle    1f              | Branch if x <= 0.5.
        fsubl   #1,fp0
        flognp1x fp0,fp0        | This is more accurate for x > 0.5.
        bras    2f
1:
        flognx  fp0,fp0         | This is more accurate for x < 0.5.
2:
        fmoved  fp0,sp@-
        moveml  sp@+,d0/d1
	RET


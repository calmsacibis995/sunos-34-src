        .data
        .asciz  "@(#)Wfuncd.s 1.1 86/09/25 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Wdefs.h"

RTENTRY(Wexpd)
	fpmoved	d0:d1,fpa0
	fpetoxd	fpa0,fpa0
	fpmoved	fpa0,d0:d1
	RET
RTENTRY(Wexp1d)
	fpmoved	d0:d1,fpa0
	fpetoxm1d	fpa0,fpa0
	fpmoved	fpa0,d0:d1
	RET
RTENTRY(Wlogd)
	fpmoved	d0:d1,fpa0
	fplognd	fpa0,fpa0
	fpmoved	fpa0,d0:d1
	RET
RTENTRY(Wlog1d)
	fpmoved	d0:d1,fpa0
	fplognp1d	fpa0,fpa0
	fpmoved	fpa0,d0:d1
	RET
RTENTRY(Wsind)
	fpmoved	d0:d1,fpa0
	fpsind	fpa0,fpa0
	fpmoved	fpa0,d0:d1
	RET
RTENTRY(Wcosd)
	fpmoved	d0:d1,fpa0
	fpcosd	fpa0,fpa0
	fpmoved	fpa0,d0:d1
	RET
RTENTRY(Watand)
	fpmoved	d0:d1,fpa0
	fpatand	fpa0,fpa0
	fpmoved	fpa0,d0:d1
	RET
RTENTRY(Wtand)
	moveml	d0/d1,sp@-
	ftand	sp@,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET
RTENTRY(Wasind)
	moveml	d0/d1,sp@-
	fasind	sp@,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET
RTENTRY(Wacosd)
	moveml	d0/d1,sp@-
	facosd	sp@,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET
RTENTRY(Wsinhd)
	moveml	d0/d1,sp@-
	fsinhd	sp@,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET
RTENTRY(Wcoshd)
	moveml	d0/d1,sp@-
	fcoshd	sp@,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET
RTENTRY(Wtanhd)
	moveml	d0/d1,sp@-
	ftanhd	sp@,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET
RTENTRY(Wpow2d)
	moveml	d0/d1,sp@-
	ftwotoxd	sp@,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET
RTENTRY(Wpow10d)
	moveml	d0/d1,sp@-
	ftentoxd	sp@,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET
RTENTRY(Wpowd)
	jsr	Mpowd
	RET
RTENTRY(Wlog2d)
	moveml	d0/d1,sp@-
|	fmoved	sp@,fp0
|	fcmps	#0r0.5,fp0
|	fjule	1f		| Branch if x <= 0.5.
|	fsubl	#1,fp0
|	flognp1x fp0,fp0	| This is more accurate for x > 0.5.
|	fmovecrx #0xd,fp1	| fp1 gets log2(e)
|	fmulx	fp1,fp0		| fp0 gets log2(x)
|	bras	2f
|1:
|	flog2x	fp0,fp0		| This is more accurate for x < 0.5.
|2:
	flog2d	sp@,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET
RTENTRY(Wlog10d)
	moveml	d0/d1,sp@-
|	fmoved	sp@,fp0
|	fcmps	#0r0.5,fp0
|	fjule	1f		| Branch if x <= 0.5.
|	fsubl	#1,fp0
|	flognp1x fp0,fp0	| This is more accurate for x > 0.5.
|	fmovecrx #0xe,fp1	| fp1 gets log10(e)
|	fmulx	fp1,fp0		| fp0 gets log10(x)
|	bras	2f
|1:
|	flog10x	fp0,fp0		| This is more accurate for x < 0.5.
|2:
	flog10d	sp@,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET

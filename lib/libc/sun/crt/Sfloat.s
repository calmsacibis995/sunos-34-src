	.data
|	.asciz	"@(#)Sfloat.s 1.1 86/09/24 Copyr 1986 Sun Micro"
	.even
	.text


|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Sdefs.h"

RTENTRY(Sfltd)
	movl	__skybase,SKYBASE 
	movw	#S_ITOD,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	SKYBASE@,d0
	movl	SKYBASE@,d1
	RET
RTENTRY(Sflts)
	movl	__skybase,SKYBASE 
	movw	#S_ITOS,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	SKYBASE@,d0
	RET
RTENTRY(Sund)
	cmpl	#0x41e00000,d0
	bges	1f
	movl	__skybase,SKYBASE 
	FDTOL(d0,d1,d0)
	bras	2f
1:
	jsr	Fund
2:
	RET
RTENTRY(Suns)
	cmpl	#0x4f000000,d0
	bges	1f
	movl	__skybase,SKYBASE 
	movw	#S_STOI,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	SKYBASE@,d0
	bras	2f
1:
	jsr	Funs
2:
	RET
RTENTRY(Sintd)
	movl	__skybase,SKYBASE 
	FDTOL(d0,d1,d0)
	RET
RTENTRY(Sints)
	movl	__skybase,SKYBASE 
	movw	#S_STOI,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	SKYBASE@,d0
	RET
RTENTRY(Sstod)
	movl	__skybase,SKYBASE 
	FSTOD(d0,d0,d1)
	RET
RTENTRY(Sdtos)
	movl	__skybase,SKYBASE 
	movw	#S_DTOS,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	SKYBASE@,d0
	RET
RTENTRY(Saddd)
	movl	__skybase,SKYBASE 
	FADDD(d0,d1,ARG2PTR@+,ARG2PTR@,d0,d1)
	RET
RTENTRY(Sadds)
	movl	__skybase,SKYBASE 
	movw	#S_SADD3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	SKYBASE@,d0
	RET
RTENTRY(Ssubd)
	movl	__skybase,SKYBASE 
	FSUBD(d0,d1,ARG2PTR@+,ARG2PTR@,d0,d1)
	RET
RTENTRY(Ssubs)
	movl	__skybase,SKYBASE 
	movw	#S_SSUB3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	IORDY
	movl	SKYBASE@,d0
	RET
RTENTRY(Smuld)
	movl	__skybase,SKYBASE 
	FMULD(d0,d1,ARG2PTR@+,ARG2PTR@,d0,d1)
	RET
RTENTRY(Smuls)
	movl	__skybase,SKYBASE 
	FMULS(d0,d1,d0)
	RET
RTENTRY(Sdivd)
	movl	__skybase,SKYBASE 
	FDIVD(d0,d1,ARG2PTR@+,ARG2PTR@,d0,d1)
	RET
RTENTRY(Sdivs)
	movl	__skybase,SKYBASE 
	FDIVS(d0,d1,d0)
	RET
RTENTRY(Ssqrd)
	movl	__skybase,SKYBASE 
	FMULD(d0,d1,d0,d1,d0,d1)
	RET
RTENTRY(Ssqrs)
	movl	__skybase,SKYBASE 
	FMULS(d0,d0,d0)
	RET

| 	Switch mode and status.

RTENTRY(Smode)
	movel	#ROUNDTODOUBLE,d0
	RET

RTENTRY(Sstatus)
	clrl	d0
	RET

RTENTRY(Scmpd)
	movl	__skybase,SKYBASE 
	movw	#S_DCMP3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	ARG2PTR@+,SKYBASE@
	movl	ARG2PTR@ ,SKYBASE@
	movew	SKYBASE@,cc
	RET

RTENTRY(Scmps)
	movl	__skybase,SKYBASE 
	movw	#S_SCMP3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movw	SKYBASE@,cc
	RET


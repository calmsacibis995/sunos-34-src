#include "DEFS.h"
#include "sky.h"

	.data
	.asciz	"@(#)skyfloat.s 1.1 86/09/24 Copyr 1983 Sun Micro"
	.even
	.text


|	Copyright (c) 1983 by Sun Microsystems, Inc.

#define READY 1:tstw SKYBASE@(-OPERAND+STATUS) ; bges 1b
|#define WAIT(n) movw #n,d0 ; 1: dbf d0,1b
#define WAIT(n) 

	.globl Sflti, Sfltis, Sfixi, Sfixis, Sdoublei, Ssinglei
	.globl Scmpi, Scmpis, Saddi, Saddis, Ssubi, Ssubis
	.globl Smuli, Smulis, Sdivi, Sdivis

	ARG2PTR	=	a0
	SKYBASE	=	a1

RTENTRY(Sflti)
	movl	__skybase,SKYBASE 
	movw	#S_ITOD,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	SKYBASE@,d0
	movl	SKYBASE@,d1
	RET
RTENTRY(Sfltis)
	movl	__skybase,SKYBASE 
	movw	#S_ITOS,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	SKYBASE@,d0
	RET
RTENTRY(Sfixi)
	movl	__skybase,SKYBASE 
	movw	#S_DTOI,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	SKYBASE@,d0
	RET
RTENTRY(Sfixis)
	movl	__skybase,SKYBASE 
	movw	#S_STOI,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	SKYBASE@,d0
	RET
RTENTRY(Sdoublei)
	movl	__skybase,SKYBASE 
	movw	#S_STOD,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	SKYBASE@,d0
	movl	SKYBASE@,d1
	RET
RTENTRY(Ssinglei)
	movl	__skybase,SKYBASE 
	movw	#S_DTOS,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	SKYBASE@,d0
	RET
RTENTRY(Scmpi)
	movl	__skybase,SKYBASE 
	movw	#S_DCMP3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	ARG2PTR@+,SKYBASE@
	movl	ARG2PTR@ ,SKYBASE@
	READY
	movw	SKYBASE@,cc
	andb	#0xfd,cc		| Clear V.
	RET
RTENTRY(Scmpis)
	movl	__skybase,SKYBASE 
	movw	#S_SCMP3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movw	SKYBASE@,cc
	andb	#0xfd,cc		| Clear V.
	RET
RTENTRY(Saddi)
	movl	__skybase,SKYBASE 
	movw	#S_DADD3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	ARG2PTR@+,SKYBASE@
	movl	ARG2PTR@ ,SKYBASE@
	READY
	movl	SKYBASE@,d0
	movl	SKYBASE@,d1
	RET
RTENTRY(Saddis)
	movl	__skybase,SKYBASE 
	movw	#S_SADD3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	SKYBASE@,d0
	RET
RTENTRY(Ssubi)
	movl	__skybase,SKYBASE 
	movw	#S_DSUB3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	ARG2PTR@+,SKYBASE@
	movl	ARG2PTR@ ,SKYBASE@
	WAIT(4)
	READY
	movl	SKYBASE@,d0
	movl	SKYBASE@,d1
	RET
RTENTRY(Ssubis)
	movl	__skybase,SKYBASE 
	movw	#S_SSUB3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	SKYBASE@,d0
	RET
RTENTRY(Smuli)
	movl	__skybase,SKYBASE 
	movw	#S_DMUL3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	ARG2PTR@+,SKYBASE@
	movl	ARG2PTR@ ,SKYBASE@
	WAIT(8)
	READY
	movl	SKYBASE@,d0
	movl	SKYBASE@,d1
	RET
RTENTRY(Smulis)
	movl	__skybase,SKYBASE 
	movw	#S_SMUL3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	SKYBASE@,d0
	RET
RTENTRY(Sdivi)
	movl	__skybase,SKYBASE 
	movw	#S_DDIV3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	ARG2PTR@+,SKYBASE@
	movl	ARG2PTR@ ,SKYBASE@
	WAIT(37)
	READY
	movl	SKYBASE@,d0
	movl	SKYBASE@,d1
	RET
RTENTRY(Sdivis)
	movl	__skybase,SKYBASE 
	movw	#S_SDIV3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	WAIT(11)
	READY
	movl	SKYBASE@,d0
	RET
RTENTRY(Ssqrtis)
	movl	__skybase,SKYBASE 
	movw	#S_SSQRT,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	WAIT(40)
	READY
	movl	SKYBASE@,d0
	RET
RTENTRY(Sexpis)
	movl	__skybase,SKYBASE 
	movw	#S_SEXP,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	WAIT(130)
	READY
	movl	SKYBASE@,d0
	RET
RTENTRY(Slogis)
	movl	__skybase,SKYBASE 
	movw	#S_SLOG,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	WAIT(50)
	READY
	movl	SKYBASE@,d0
	RET
RTENTRY(Ssinis)
	movl	__skybase,SKYBASE 
	movw	#S_SSIN,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	WAIT(90)
	READY
	movl	SKYBASE@,d0
	RET
RTENTRY(Scosis)
	movl	__skybase,SKYBASE 
	movw	#S_SCOS,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	WAIT(90)
	READY
	movl	SKYBASE@,d0
	RET
RTENTRY(Stanis)
	movl	__skybase,SKYBASE 
	movw	#S_STAN,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	WAIT(90)
	READY
	movl	SKYBASE@,d0
	RET
RTENTRY(Satanis)
	movl	__skybase,SKYBASE 
	movw	#S_SATAN,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	WAIT(130)
	READY
	movl	SKYBASE@,d0
	RET
RTENTRY(Spowis)
	movl	__skybase,SKYBASE 
	movw	#S_SPOW,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	WAIT(50)
	READY
	movl	d1,SKYBASE@
	WAIT(130)
	READY
	movl	SKYBASE@,d0
	RET


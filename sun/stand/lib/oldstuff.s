	.data
	.asciz "@(#)oldstuff.s 1.1 9/25/86 Copyright Sun Micro"
	.even
	.text

|	Copyright (c) 1983 by Sun Microsystems, Inc.

	.text
	.globl  fvaddi, fvsubi, fvmuli, fvdivi, fvcmpi
	.globl  fvaddis, fvsubis, fvmulis, fvdivis, fvcmpis
	.globl	fvflti, fvfltis, fvfixi, fvfixis, fvdoublei, fvsinglei

| call-sequence compatability + register saving
#include "DEFS.h"
RTENTRY(faddi)
	movl	a0,sp@- ;  movl	a1,sp@-
#if PROF
	lea	PARAM,a0
#else
	lea	PARAMX(8),a0	| affected by a0,a1 pushes
#endif
	jsr	fvaddi
	movl	sp@+,a1 ;  movl sp@+,a0
	RETN(8)

RTENTRY(fsubi)
	movl	a0,sp@- ;  movl	a1,sp@-
#if PROF
	lea	PARAM,a0
#else
	lea	PARAMX(8),a0	| affected by a0,a1 pushes
#endif
	jsr	fvsubi
	movl	sp@+,a1 ;  movl sp@+,a0
	RETN(8)

RTENTRY(fmuli)
	movl	a0,sp@- ;  movl	a1,sp@-
#if PROF
	lea	PARAM,a0
#else
	lea	PARAMX(8),a0	| affected by a0,a1 pushes
#endif
	jsr	fvmuli
	movl	sp@+,a1 ;  movl sp@+,a0
	RETN(8)

RTENTRY(fdivi)
	movl	a0,sp@- ;  movl	a1,sp@-
#if PROF
	lea	PARAM,a0
#else
	lea	PARAMX(8),a0	| affected by a0,a1 pushes
#endif
	jsr	fvdivi
	movl	sp@+,a1 ;  movl sp@+,a0
	RETN(8)

RTENTRY(fcmpi)
	movl	a0,sp@- ;  movl	a1,sp@-
#if PROF
	lea	PARAM,a0
#else
	lea	PARAMX(8),a0	| affected by a0,a1 pushes
#endif
	jsr	fvcmpi
	movl	sp@+,a1 ;  movl sp@+,a0
	RETN(8)

RTENTRY(faddis)
	movl	a1,sp@- 
	jsr	fvaddis
	movl	sp@+,a1
	RET

RTENTRY(fsubis)
	movl	a1,sp@- 
	jsr	fvsubis
	movl	sp@+,a1
	RET

RTENTRY(fmulis)
	movl	a1,sp@- 
	jsr	fvmulis
	movl	sp@+,a1
	RET

RTENTRY(fdivis)
	movl	a1,sp@- 
	jsr	fvdivis
	movl	sp@+,a1
	RET

RTENTRY(fcmpis)
	movl	a1,sp@- 
	jsr	fvcmpis
	movl	sp@+,a1
	RET

RTENTRY(ffixis)
	movl	a1,sp@- 
	jsr	fvfixis
	movl	sp@+,a1
	RET

RTENTRY(ffixi)
	movl	a1,sp@- 
	jsr	fvfixi
	movl	sp@+,a1
	RET

RTENTRY(ffltis)
	movl	a1,sp@- 
	jsr	fvfltis
	movl	sp@+,a1
	RET

RTENTRY(fflti)
	movl	a1,sp@- 
	jsr	fvflti
	movl	sp@+,a1
	RET

RTENTRY(fdoublei)
	movl	a1,sp@- 
	jsr	fvdoublei
	movl	sp@+,a1
	RET

RTENTRY(fsinglei)
	movl	a1,sp@- 
	jsr	fvsinglei
	movl	sp@+,a1
	RET


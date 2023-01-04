	.data
	.asciz	"@(#)rel1support.s 1.1 86/09/24 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

	.text
        
#include "fpcrtdefs.h"
	
/*	OBSOLETE FEATURES FROM RELEASE 1.X	*/

OBSOLETE(floatflavor)
OBSOLETE(ieeeused)
	.asciz "IEEE software|SKY"
 
| call-sequence compatability + register saving
RTOBSOLETE(faddi)
	movl	a0,sp@- ;  movl	a1,sp@-
#if PROF
	lea	PARAM,a0
#else
	lea	PARAMX(8),a0	| affected by a0,a1 pushes
#endif
	jsr	fvaddi
	movl	sp@+,a1 ;  movl sp@+,a0
	RETN(8)

RTOBSOLETE(fsubi)
	movl	a0,sp@- ;  movl	a1,sp@-
#if PROF
	lea	PARAM,a0
#else
	lea	PARAMX(8),a0	| affected by a0,a1 pushes
#endif
	jsr	fvsubi
	movl	sp@+,a1 ;  movl sp@+,a0
	RETN(8)

RTOBSOLETE(fmuli)
	movl	a0,sp@- ;  movl	a1,sp@-
#if PROF
	lea	PARAM,a0
#else
	lea	PARAMX(8),a0	| affected by a0,a1 pushes
#endif
	jsr	fvmuli
	movl	sp@+,a1 ;  movl sp@+,a0
	RETN(8)

RTOBSOLETE(fdivi)
	movl	a0,sp@- ;  movl	a1,sp@-
#if PROF
	lea	PARAM,a0
#else
	lea	PARAMX(8),a0	| affected by a0,a1 pushes
#endif
	jsr	fvdivi
	movl	sp@+,a1 ;  movl sp@+,a0
	RETN(8)

RTOBSOLETE(fcmpi)
	movl	a0,sp@- ;  movl	a1,sp@-
#if PROF
	lea	PARAM,a0
#else
	lea	PARAMX(8),a0	| affected by a0,a1 pushes
#endif
	jsr	fvcmpi
	movl	sp@+,a1 ;  movl sp@+,a0
	RETN(8)

RTOBSOLETE(faddis)
	movl	a1,sp@- 
	jsr	fvaddis
	movl	sp@+,a1
	RET

RTOBSOLETE(fsubis)
	movl	a1,sp@- 
	jsr	fvsubis
	movl	sp@+,a1
	RET

RTOBSOLETE(fmulis)
	movl	a1,sp@- 
	jsr	fvmulis
	movl	sp@+,a1
	RET

RTOBSOLETE(fdivis)
	movl	a1,sp@- 
	jsr	fvdivis
	movl	sp@+,a1
	RET

RTOBSOLETE(fcmpis)
	movl	a1,sp@- 
	jsr	fvcmpis
	movl	sp@+,a1
	RET

RTOBSOLETE(ffixis)
	movl	a1,sp@- 
	jsr	fvfixis
	movl	sp@+,a1
	RET

RTOBSOLETE(ffixi)
	movl	a1,sp@- 
	jsr	fvfixi
	movl	sp@+,a1
	RET

RTOBSOLETE(ffltis)
	movl	a1,sp@- 
	jsr	fvfltis
	movl	sp@+,a1
	RET

RTOBSOLETE(fflti)
	movl	a1,sp@- 
	jsr	fvflti
	movl	sp@+,a1
	RET

RTOBSOLETE(fdoublei)
	movl	a1,sp@- 
	jsr	fvdoublei
	movl	sp@+,a1
	RET

RTOBSOLETE(fsinglei)
	movl	a1,sp@- 
	jsr	fvsinglei
	movl	sp@+,a1
	RET

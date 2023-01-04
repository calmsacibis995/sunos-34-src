/*	@(#)DEFS.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*	@(#)DEFS.h 1.3 83/08/23 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */



#ifdef PROF
	.globl  mcount
#define MCOUNT		 lea 277$,a0;\
		 .data; 277$: .long 0; .text;\
			 jsr mcount
#define LINK		link  a6,#0
#define RTMCOUNT	moveml	#0xC0C0,sp@-; MCOUNT; moveml sp@+,#0x0303
#define RET	unlk a6; rts
#define RETN(n)	unlk a6; rts #n
#define PARAMX( n )	a6@(8+n)

#else not PROF

#define MCOUNT
#define RTMCOUNT
#define LINK
#define RET	rts
#define RETN(n) rts #n
#define PARAMX( n )	sp@(4+n)

#endif not PROF

#define	ENTRY(x)	.globl _/**/x; _/**/x: LINK; MCOUNT
#define	RTENTRY(x)	.globl x; x: LINK; RTMCOUNT
#define PARAM0	PARAMX(-4)
#define PARAM	PARAMX(0)
#define PARAM2	PARAMX(4)
#define PARAM3	PARAMX(8)

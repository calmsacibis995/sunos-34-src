/*	@(#)param.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 *
 * This file is intended to contain the
 * specific details of a given architecture.
 */

/*
 * Machine dependent constants for Sun-2.
 */
#define	NBPG	2048		/* bytes/page */
#define	PGOFSET	(NBPG-1)	/* byte offset into page */
#define	PGSHIFT	11		/* LOG2(NBPG) */

#define	NBSG	32768		/* bytes/segment */
#define	SGOFSET	(NBSG-1)	/* byte offset into segment */
#define	SGSHIFT	15		/* LOG2(NBSG) */
#define	VSGSHIFT 17		/* LOG2(Virtual NBSG) */

#define	CLSIZE		1
#define	CLSIZELOG2	0

#define	SSIZE		1	/* initial stack size/NBPG */
#define	SINCR		1	/* increment of stack/NBPG */

#define	UADDR		0x3000	/* u-area virtual address */
#define	UPAGES		2	/* pages of u-area NOT including red zone */

#define	KERNSTACK	CLBYTES	/* size of kernel stack in u-area */

#define	KERNELBASE	0x0	/* start of kernel mapping */

/*
 * Some macros for units conversion
 */
/* Core clicks (2048 bytes) to segments and vice versa */
#define	ctos(x)	(((x)+63)>>(VSGSHIFT-PGSHIFT))
#define	stoc(x)	((x)<<(VSGSHIFT-PGSHIFT))

/* Page number to segment number */
#define	ptos(x)	((x)>>(SGSHIFT-PGSHIFT))

/* Core clicks (2048 bytes) to disk blocks and vice versa */
#define	ctod(x)	((x)<<2)
#define	dtoc(x)	(((x)+3)>>2)
#define	dtob(x)	((x)<<9)

/* clicks to bytes */
#define	ctob(x)	((x)<<PGSHIFT)

/* bytes to clicks */
#define	btoc(x)	((((unsigned)(x)+(NBPG-1))>>PGSHIFT))

/*
 * Macros to decode processor status word.
 */
#define	USERMODE(ps)	(((ps) & SR_SMODE) == 0)
#define	BASEPRI(ps)	(((ps) & SR_INTPRI) == 0)

/*
 * Delay units are in microseconds.
 */
#define	DELAY(n)	\
{ \
	extern int cpudelay; \
	register int N = (((n)<<4) >> cpudelay); \
 \
	while (--N > 0) ; \
}

#define	CDELAY(c, n)	\
{ \
	extern int cpudelay; \
	register int N = (((n)<<3) >> cpudelay); \
 \
	while (--N > 0) \
		if (c) \
			break; \
}

/* Define alignment macro for portability among product families */
#define shm_alignment	CLBYTES		/* shared memory alignment */

/*	@(#)param.h 1.1 84/12/20 SMI	*/

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

#define	SGSHIFT	15		/* LOG2(bytes/segment) */

#define	CLSIZE		1
#define	CLSIZELOG2	0

#define	SSIZE	1		/* initial stack size/NBPG */
#define	SINCR	1		/* increment of stack/NBPG */

#define	UPAGES	3		/* pages of u-area */

/*
 * Some macros for units conversion
 */
/* Core clicks (2048 bytes) to segments and vice versa */
#define	ctos(x)	(((x)+15)>>(SGSHIFT-PGSHIFT))
#define	stoc(x)	((x)<<(SGSHIFT-PGSHIFT))

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
#define	DELAY(n)	{ register int N = (n)>>1; while (--N > 0); }
#define	CDELAY(c, n)	{ register N = (n)>>2; while (--N > 0) if(c) break; }

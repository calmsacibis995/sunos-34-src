/*      @(#)param.h 1.1 86/09/25 SMI; from UCB 6.1 83/07/29       */

/*
 * Machine dependent constants for vax.
 */
#define	NBPG	512			/* bytes/page */
#define	PGOFSET	(NBPG-1)		/* byte offset into page */
#define	PGSHIFT	9			/* LOG2(NBPG) */

#define	CLSIZE		2
#define	CLSIZELOG2	1

#define	SSIZE	4			/* initial stack size/NBPG */
#define	SINCR	4			/* increment of stack/NBPG */

#define	KERNELBASE	0x80000000	/* start of kernel mapping */

#define	UPAGES		10		/* u-area pages (w/o red zone) */
#define	KERNSTACK	(CLBYTES*3)	/* size of kernel stack in u-area */
#define	UADDR	(KERNELBASE-(UPAGES*NBPG))/* virtual address of u area */

/*
 * Some macros for units conversion
 */
/* Core clicks (512 bytes) to segments and vice versa */
#define	ctos(x)	(x)
#define	stoc(x)	(x)

/* Core clicks (512 bytes) to disk blocks */
#define	ctod(x)	(x)
#define	dtoc(x)	(x)
#define	dtob(x)	((x)<<9)

/* clicks to bytes */
#define	ctob(x)	((x)<<9)

/* bytes to clicks */
#define	btoc(x)	((((unsigned)(x)+511)>>9))

/*
 * Macros to decode processor status word.
 */
#define	USERMODE(ps)	(((ps) & PSL_CURMOD) == PSL_CURMOD)
#define	BASEPRI(ps)	(((ps) & PSL_IPL) == 0)

#define	DELAY(n)	{ register int N = (n); while (--N > 0); }

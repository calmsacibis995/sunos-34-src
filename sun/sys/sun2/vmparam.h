/*	@(#)vmparam.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Machine dependent constants for Sun-2
 */

/*
 * USRTEXT is the start of the user text/data space, while USRSTACK
 * is the top (end) of the user stack.  LOWPAGES and HIGHPAGES are
 * the number of pages from the beginning of the P0 region to the
 * beginning of the text and from the end of the stack to the end of the P1
 * region respectively.
 */
#define	USRTEXT		0x2000		/* for new (Sun-3 like) a.outs */
#define	OUSRTEXT	0x8000		/* for old Sun-2 a.outs */
#define	USRSTACK	0x1000000
#define	P1PAGES		btoc(USRSTACK)
#define	LOWPAGES	btoc(USRTEXT)
#define	HIGHPAGES	0

/*
 * Virtual memory related constants
 */
#define	SLOP	32
#define	MAXTSIZ		btoc(8*1024*1024)	/* max text size (clicks) */
#define	MAXDSIZ		(btoc(USRSTACK)-SLOP)	/* max data size (clicks) */
#define	MAXSSIZ		(btoc(USRSTACK)-SLOP)	/* max stack size (clicks) */

/*
 * Sizes of the system and user portions of the system page table.
 * Note that they point into the kernel virtual address space, hence
 *   what they describe is the size of the table space, not the size of
 *   the space which can be mapped.
 */
#define	SYSPTSIZE	(0x100000/NBPG)
#define	USRPTSIZE 	(0x400000/NBPG)

/*
 * The size of the clock loop.
 */
#define	LOOPPAGES	(maxfree - firstfree)

/*
 * The time for a process to be blocked before being very swappable.
 * This is a number of seconds which the system takes as being a non-trivial
 * amount of real time.  You probably shouldn't change this;
 * it is used in subtle ways (fractions and multiples of it are, that is, like
 * half of a ``long time'', almost a long time, etc.)
 * It is related to human patience and other factors which don't really
 * change over time.
 */
#define	MAXSLP 		20

/*
 * A swapped in process is given a small amount of core without being bothered
 * by the page replacement algorithm.  Basically this says that if you are
 * swapped in you deserve some resources.  We protect the last SAFERSS
 * pages against paging and will just swap you out rather than paging you.
 * Note that each process has at least UPAGES+CLSIZE pages which are not
 * paged anyways (this is currently 2+1=3 pages or 6k bytes), so this
 * number just means a swapped in process is given around 22k bytes.
 */
#define	SAFERSS		8		/* nominal ``small'' resident set size
					   protected against replacement */

/*
 * DISKRPM is used to estimate the number of paging i/o operations
 * which one can expect from a single disk controller.
 */
#define	DISKRPM		60

/*
 * Klustering constants.  Klustering is the gathering
 * of pages together for pagein/pageout, while clustering
 * is the treatment of hardware page size as though it were
 * larger than it really is.
 *
 * KLMAX gives maximum cluster size in CLSIZE page (cluster-page)
 * units.  Note that KLMAX*CLSIZE must be a divisor of DMMIN in
 * autoconf.c and <= MAXPHYS/CLBYTES in vm_swp.c.
 */

#define	KLMAX	(8/CLSIZE)
#define	KLSEQL	(4/CLSIZE)		/* in klust if vadvise(VA_SEQL) */
#define	KLIN	(2/CLSIZE)		/* default data/stack in klust */
#define	KLTXT	(1/CLSIZE)		/* default text in klust */
#define	KLOUT	(8/CLSIZE)

/*
 * KLSDIST is the advance or retard of the fifo reclaim for sequential
 * processes data space.
 */
#define	KLSDIST	3		/* klusters advance/retard for seq. fifo */

/*
 * Paging thresholds (see vm_sched.c).
 * Strategy of 3/17/83:
 *	lotsfree is 1/8 of memory free.
 *	desfree is 100k bytes, but at most 1/16 of memory
 *	minfree is 32k bytes, but at most 1/2 of desfree
 */
#define	LOTSFREEFRACT	8
#define	DESFREE		(100 * 1024)
#define	DESFREEFRACT	16
#define	MINFREE		(32 * 1024)
#define	MINFREEFRACT	2

/*
 * Paged text files that are less than PGTHRESH bytes may be swapped
 * in instead of paged in.
 */
#define PGTHRESH	(200 * 1024)

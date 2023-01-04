/*	@(#)cpu.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 * This file is intended to contain the specific details
 * of various implementations of a given architecture.
 */

#define	CPU_ARCH	0xf0		/* mask for architecture bits */
#define	SUN2_ARCH	0x00		/* 0 for backwards compatibility */

#define	CPU_MACH	0x0f		/* mask for machine implementation */
#define	MACH_120	0x01		/* generic for Multibus based system */
#define	MACH_50		0x02	 	/* generic for VMEbus based system */

#define	CPU_SUN2_120	(SUN2_ARCH + MACH_120)	/* cpu value for mach 1 */
#define	CPU_SUN2_50	(SUN2_ARCH + MACH_50)	/* cpu value for mach 2 */

#if defined(KERNEL) && !defined(LOCORE)
int cpu;				/* machine type we are running on */
int dvmasize;				/* usable dvma size in clicks */
int fbobmemavail;			/* video copy memory is available */
#endif defined(KERNEL) && !defined(LOCORE)

#ifndef LOCORE
/*
 * The context structure is used to allocate
 * contexts and maintains the pmeg allocation
 * information for the context.
 */
struct	context {
	int	ctx_time;		/* pseudo-time for ctx lru */
	u_short	ctx_context;		/* bits to load into context register */
	short	ctx_tdmax;		/* max text or data seg used */
	short	ctx_smin;		/* min stack segment used */
	struct	proc *ctx_procp;	/* back pointer to proc structure */
	u_char	ctx_pmeg[NSEGMAP];	/* pmeg allocation for this context */
};

/*
 * CSEG is the virtual segment reserved for temporary operations.
 * We use the segment immediately before the start of debugger area.
 */
#include "../debug/debug.h"
#define	CSEG	((DEBUGSTART - NBSG) >> SGSHIFT)

/*
 * The pmeg structure allocates the
 * hardware page map entry groups.
 */
struct	pmeg {
	struct	pmeg *pm_forw;		/* forward link */
	struct	pmeg *pm_back;		/* backward link */
	struct	proc *pm_procp; 	/* back pointer to proc using */
	struct	pte *pm_pte;		/* pointer to pte's mapping */
	short	pm_seg;			/* seg within process address space */
	short	pm_count;		/* number of valid pte's in group */
	int	pm_time;		/* last "time" pmeg was unloaded */
};

#ifdef KERNEL
extern	struct context context[];
extern	struct pmeg pmeg[];
extern	struct pmeg pmeghead;
extern	int kernpmeg;
#endif KERNEL
#endif !LOCORE

/*
 * Various I/O space related constants
 */
#define	MBMEM_SIZE	0x100000
#define MBIO_SIZE	0x010000
#define VME0_SIZE	0x800000
#define VME8_SIZE	0x800000

/*
 * Basic DVMA space sizes for the different implementations.
 * The actual usable dvma size (in clicks) is given by the
 * dvmasize variable declared above.
 */
#define	SUN2_120_DVMASIZE	0x40000
#define	SUN2_50_DVMASIZE	0x100000

/*
 * FBSIZE is the amount of memory we will use for the frame buffer 
 * copy region if the copy mode of the frame buffer is to be used
 */
#define FBSIZE   0x20000	/* size of frame buffer memory region */
#define OBFBADDR 0x100000	/* location of frame buffer in memory */

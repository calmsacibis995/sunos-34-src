/*      @(#)cpu.h 1.6 87/02/16 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 * This file is intended to contain the specific details
 * of various implementations of a given architecture.
 */

#define	CPU_ARCH	0xf0		/* mask for architecture bits */
#define	SUN3_ARCH	0x10		/* arch value for Sun 3 */

#define	CPU_MACH	0x0f		/* mask for machine implementation */
#define	MACH_160	0x01
#define	MACH_50		0x02
#define	MACH_260	0x03
#define	MACH_110	0x04
#define MACH_60		0x07

#define CPU_SUN3_160	(SUN3_ARCH + MACH_160)
#define CPU_SUN3_50	(SUN3_ARCH + MACH_50)
#define CPU_SUN3_260	(SUN3_ARCH + MACH_260)
#define CPU_SUN3_110	(SUN3_ARCH + MACH_110)
#define CPU_SUN3_60	(SUN3_ARCH + MACH_60)

#if defined(KERNEL) && !defined(LOCORE)
int cpu;				/* machine type we are running on */
int dvmasize;				/* usable dvma size in clicks */
int fbobmemavail;			/* video copy memory is available */
#ifdef SUN3_260
int vac;				/* there is virtual address cache */
#endif SUN3_260
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
#define	VME16_BASE	0xFFFF0000
#define	VME16_SIZE	(1<<16)
#define	VME16_MASK	(VME16_SIZE-1)

#define	VME24_BASE	0xFF000000
#define	VME24_SIZE	(1<<24)
#define	VME24_MASK	(VME24_SIZE-1)

/*
 * The Basic DVMA space size for all Sun-3 implementations
 * is given by DVMASIZE.  The actual usable dvma size
 * (in clicks) is given by the dvmasize variable declared
 * above (for compatibility with Sun-2).
 */
#define	DVMASIZE	0x100000

/*
 * FBSIZE is the amount of memory we will use for the frame buffer 
 * copy region if the copy mode of the frame buffer is to be used.
 */
#define FBSIZE   0x20000	/* size of frame buffer memory region */
#define OBFBADDR 0x100000	/* location of frame buffer in memory */

/*	@(#)mmu.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* 
 * Sun-2 memory management.
 */
#define	KCONTEXT 0		/* Kernel's context */
#define	NCONTEXT 8		/* Number of contexts */
#define	CONTEXTMASK	(NCONTEXT-1)

/*
 * Hardware segment and page registers and constants.
 */
#define	NSEGMAP	512		/* # of segments per context */
#define	SEGINV	(NPMEG-1)	/* invalid segment - no access */
#define	NPAGSEG 16		/* # of pages per segment */
#define	NPME	4096		/* number of hardware page map entries */
#define	NPMEG	(NPME/NPAGSEG)	/* # of pme groups (segment allocation) */

/*
 * Function code register values.
 */
#define	FC_UD	1		/* user data */
#define	FC_UP	2		/* user program */
#define	FC_MAP	3		/* Sun-2 memory maps */
#define	FC_SD	5		/* supervisor data */
#define	FC_SP	6		/* supervisor program */
#define	FC_CPU	7		/* cpu space */


/*
 * FC_MAP addresses
 */
#define	SUPCONTEXTOFF	6	/* supervisor context register */
#define	USERCONTEXTOFF	7	/* user context register */
#define	SMAPOFF		5	/* offset to segment map entries */

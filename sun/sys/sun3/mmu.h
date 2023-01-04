/*      @(#)mmu.h 1.1 86/09/25 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* 
 * Sun-3 memory management.
 */
#define KCONTEXT	 0	/* Kernel context (no valid user pages) */
#define	NCONTEXT	 8	/* Number of contexts */
#define	CONTEXTMASK	(NCONTEXT-1)

/*
 * Hardware segment and page registers and constants.
 */
#define	NSEGMAP	2048		/* # of segments per context */
#define	SEGINV	(NPMEG-1)	/* invalid pmeg - no access */
#define	NPAGSEG 16		/* # of pages per segment */
#define	NPME	4096		/* number of hardware page map entries */
#define	NPMEG	(NPME/NPAGSEG)	/* # of pme groups (segment allocation) */

/*
 * Function code register values.
 */
#define	FC_UD	1		/* user data */
#define	FC_UP	2		/* user program */
#define	FC_MAP	3		/* Sun-3 memory maps */
#define	FC_SD	5		/* supervisor data */
#define	FC_SP	6		/* supervisor program */
#define	FC_CPU	7		/* cpu space */

/*
 * FC_MAP base addresses
 */
#define	IDPROMBASE	0x00000000	/* id prom base */
#define	PAGEBASE	0x10000000	/* page map base */
#define	SEGMENTBASE	0x20000000	/* segment map base */
#define	CONTEXTBASE	0x30000000	/* context map base */

#define IDPROMSIZE	0x20		/* size of id prom in bytes */

/*
 * Masks for relevant bits of virtual address
 * when accessing control space devices
 */
#define	PAGEADDRBITS	0x0FFFE000	/* page map virtual address mask */
#define	SEGMENTADDRBITS	0x0FFE0000	/* segment map virtual address mask */

/*
 * 68020 Cache Control Register
 */
#define CACHE_ENABLE	0x1	/* enable the cache */
#define CACHE_FREEZE	0x2	/* freeze the cache */
#define CACHE_CLRENTRY	0x4	/* clear entry specified by cache addr reg */
#define CACHE_CLEAR	0x8	/* clear entire cache */

/*
 * Sun-3 Virtual Address Cache (VAC)
 * The VAC h/w has a counter cycles bits 4 through bit 8.  We cycle
 * bit 9 through bit 15 for 64KB VAC.  For 128KB VAC, we cycle bit 9
 * through bit 16.  (There is only one size of VAC for SUN-3 260.
 * If there is more than one VAC size, VAC_FLUSH_HIGHBIT should be
 * a variable whose value is set at boot time.)
 */
#define VAC_FLUSH_BASE		0xA0000000
#define VAC_FLUSH_LOWBIT	9
#define VAC_FLUSH_HIGHBIT	15	/* 64KB VAC */
#define VAC_FLUSH_INCRMNT	512	/* 2 << VAC_LOW_BIT */
#define VAC_UNIT_ADDRBITS	0x0FFFFE00
#define VAC_UNIT_MASK		0x000001FF

/*
 * Number of cycles to flush a context, segment, and page.
 * For a context flush or segment flush, we loop
 * 2 << (VAC_HIGH_BIT - VAC_LOW_BIT + 1) times, and for a page flush,
 * we loop 2 << (PHSHIFT - VAC_LOW_BIT) times.  
 */
#define VAC_CTXFLUSH_COUNT	128
#define VAC_SEGFLUSH_COUNT	128
#define VAC_PAGEFLUSH_COUNT	16

/* contants to do a flush */
/* VAC_FLUSHALL is used for debugging only for SUN3_260 */
#define VAC_FLUSHALL		0
#define VAC_CTXFLUSH		0x01
#define VAC_SEGFLUSH		0x03
#define VAC_PAGEFLUSH		0x02

/*
 * VAC Read/Write Cache Tags.
 * R/W tag is for one line in the cache, which has 16 bytes.
 * VAC_RWTAG_COUNT is 2 ^ (VAC_RWTAG_HIGHBIT - VAC_RWTAG_LOWBIT + 1) - 1
 */
#define VAC_RWTAG_BASE		0x80000000
#define VAC_RWTAG_LOWBIT	4
#define VAC_RWTAG_HIGHBIT	15	/* 64KB VAC */
#define VAC_RWTAG_INCRMNT	16	/* 2 ^ VAC_RWTAG_LOWBIT */
#define VAC_RWTAG_COUNT		4096

/*
 * Block Copy Read and Block Copy Write
 */
#define VAC_LINESIZE		16
#define VAC_LINE_SHIFT		2
#define VAC_LINE_RESIDU		3
#define VAC_BLOCK_CPCMD		0xB0000000
#define VAC_BLOCK_OFF		0x0FFFFFFF

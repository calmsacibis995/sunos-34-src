/*      @(#)pte.h 1.1 86/09/25 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Sun 3 hardware page table entry
 *
 * There are two major kinds of pte's: those which have ever existed (and are
 * thus either now in core or on the swap device), and those which have
 * never existed, but which will be filled on demand at first reference.
 * There is a structure describing each.
 * Note that (pg_v && pg_fod) indicates a special (eg, mmapped) page: 
 * it is not paged out (not dirty), nor filled in (still valid).
 */

#ifndef LOCORE
struct pte {
	unsigned int	pg_v:1;		/* valid bit */
	unsigned int	pg_prot:2;	/* access protection */
	unsigned int	pg_nc:1;	/* no cache bit */
	unsigned int	pg_type:2;	/* page type */
	unsigned int	pg_r:1;		/* referenced */
	unsigned int	pg_m:1;		/* modified */
	unsigned int	:3;
	unsigned int	pg_fod:1;	/* is fill on demand (=0) */
	unsigned int	:1;
	unsigned int	pg_pfnum:19;	/* page frame number */
};

struct fpte {
	unsigned int	pg_v:1;		/* valid bit */
	unsigned int	pg_prot:2;	/* access protection */
	unsigned int	pg_nc:1;	/* no cache bit */
	unsigned int	pg_type:2;	/* page type */
	unsigned int	pg_fileno:5;	/* file mapped from or TEXT or ZERO */
	unsigned int	pg_fod:1;	/* is fill on demand (=1) */
	unsigned int	pg_blkno:20;	/* file system block number */
};
#endif

#define	PG_V		0x80000000	/* page is valid */
#define	PG_PROT		0x60000000	/* access protection mask */
#define		PG_W	0x40000000	/* write enable bit */
#define		PG_S	0x20000000	/* system page */
#define	PG_NC		0x10000000	/* no cache bit */
#define	PG_TYPE		0x0C000000	/* page type mask */
#define	PG_R		0x02000000	/* page referenced bit */
#define	PG_M		0x01000000	/* page modified bit */
#define	PG_FOD		0x00100000	/* page fill-on-demand bit */
#define	PG_PFNUM	0x0C07FFFF	/* page # mask - XXX includes type */

#define	PG_FZERO	(NOFILE)
#define	PG_FTEXT	(NOFILE+1)
#define	PG_FMAX		(PG_FTEXT)

#define	PG_KW		(PG_S|PG_W)
#define	PG_KR		PG_S
#define	PG_UW		PG_W		/* kernel can still access */
#define	PG_UWKW		PG_UW
#define	PG_UR		0		/* kernel can still access */
#define	PG_URKR		PG_UR
#define	PG_UPAGE	PG_KW		/* sun3 u pages not user accessable */

#define	PGT_MASK	(3<<26)

#define	PGT_OBMEM	(0<<26)		/* onboard memory */
#define	PGT_OBIO	(1<<26)		/* onboard I/O */
#define PGT_VME_D16	(2<<26)		/* VMEbus 16-bit data */
#define	PGT_VME_D32	(3<<26)		/* VMEbus 32-bit data */

/*
 * Pte related macros
 */
#define	dirty(pte)	((pte)->pg_fod == 0 && (pte)->pg_pfnum && (pte)->pg_m)

#if defined(KERNEL) && !defined(LOCORE)
struct	pte *vtopte();

/* utilities defined in locore.s */
extern	struct pte Sysmap[];
extern	struct pte Usrptmap[];
extern	struct pte usrpt[];
extern	struct pte Swapmap[];
extern	struct pte Forkmap[];
extern	struct pte Xswapmap[];
extern	struct pte Xswap2map[];
extern	struct pte Pushmap[];
extern	struct pte Vfmap[];
extern	struct pte mmap[];
extern	struct pte CMAP1[];
extern	struct pte CMAP2[];
#endif defined(KERNEL) && !defined(LOCORE)

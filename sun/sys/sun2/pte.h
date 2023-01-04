/*	@(#)pte.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Sun hardware page table entry
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
unsigned int	pg_v:1,			/* valid bit */
		pg_prot:5,		/* access control */
		pg_fod:1,		/* is fill on demand (=0) */
		:5,
		pg_r:1,			/* referenced */
		pg_m:1,			/* modified */
		pg_type:2,		/* page type */
		pg_pfnum:16;		/* page frame number */
};

struct fpte {
unsigned int	pg_v:1,
		pg_prot:5,
		pg_fod:1,		/* is fill on demand (=1) */
		pg_fileno:5,		/* file mapped from or TEXT or ZERO */
		pg_blkno:20;		/* file system block number */
};
#endif

#define	PG_V		0x80000000
#define	PG_PROT		0x7c000000
#define	PG_R		0x00080000
#define	PG_M		0x00040000
#define	PG_FOD		0x02000000
#define	PG_PFNUM	0x0003ffff	/* XXX - includes type field */

#define	PG_FZERO	(NOFILE)
#define	PG_FTEXT	(NOFILE+1)
#define	PG_FMAX		(PG_FTEXT)

#define	PG_NOACC	0
#define	PG_KW		0x70000000
#define	PG_KR		0x50000000
#define	PG_UW		0x7c000000
#define	PG_URKR		0x58000000
#define	PG_UPAGE	PG_KW		/* sun2 u pages not user accessable */

#define	PGT_MASK	(3<<16)

#define	PGT_OBMEM	(0<<16)
#define	PGT_OBIO	(1<<16)

#define	PGT_MBMEM	(2<<16)		/* cpu == CPU_SUN2_120 */
#define PGT_VME0	(2<<16)		/* cpu == CPU_SUN2_50 */
#define PGT_DVMABUS	(2<<16)
#define PGT_MBMEM_VME0	(2<<16)

#define	PGT_MBIO	(3<<16)		/* cpu == CPU_SUN2_120 */
#define	PGT_VME8	(3<<16)		/* cpu == CPU_SUN2_50 */
#define	PGT_MBIO_VME8	(3<<16)

/*
 * Pte related macros
 */
#define	dirty(pte)	((pte)->pg_fod == 0 && (pte)->pg_pfnum && (pte)->pg_m)

#ifndef LOCORE
#ifdef KERNEL
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
extern	struct pte msgbufmap[];
extern	struct pte CMAP1[];
extern	struct pte CMAP2[];
#endif
#endif

/*	@(#)mbuf.h 1.1 86/09/25 SMI; from UCB 4.18 83/03/25	*/

/*
 * Constants related to memory allocator.
 */
#define	MSIZE		128			/* size of an mbuf */
#define	MMINOFF		12			/* mbuf header length */
#define	MTAIL		4
#define	MMAXOFF		(MSIZE-MTAIL)		/* offset where data ends */
#define	MLEN		(MSIZE-MMINOFF-MTAIL)	/* mbuf data length */
#define	NMBCLUSTERS	256			/* # of virt pages for mbufs */

/*
 * Macros for type conversion
 */

/* address in mbuf to mbuf head */
#define	dtom(x)		((struct mbuf *)((int)x & ~(MSIZE-1)))

/* mbuf head, to typed data */
#define	mtod(x,t)	((t)((int)(x) + (x)->m_off))

struct mbuf {
	struct	mbuf *m_next;		/* next buffer in chain */
	u_long	m_off;			/* offset of data */
	short	m_len;			/* amount of data in this mbuf */
	short	m_type;			/* mbuf type (0 == free) */
	union {
		u_char	mun_dat[MLEN];	/* data storage */
		struct {
			short	mun_cltype;	/* "cluster" type */
			int	(*mun_clfun)();
			int	mun_clarg;
			int	(*mun_clswp)();
		} mun_cl;
	} m_un;
	struct	mbuf *m_act;		/* link in higher-level mbuf list */
};
#define	m_dat	m_un.mun_dat
#define	m_cltype m_un.mun_cl.mun_cltype
#define	m_clfun	m_un.mun_cl.mun_clfun
#define	m_clarg	m_un.mun_cl.mun_clarg
#define	m_clswp	m_un.mun_cl.mun_clswp

/* mbuf types */
#define	MT_FREE		0	/* should be on free list */
#define	MT_DATA		1	/* dynamic (data) allocation */
#define	MT_HEADER	2	/* packet header */
#define	MT_SOCKET	3	/* socket structure */
#define	MT_PCB		4	/* protocol control block */
#define	MT_RTABLE	5	/* routing tables */
#define	MT_HTABLE	6	/* IMP host tables */
#define	MT_ATABLE	7	/* address resolution tables */
#define	MT_SONAME	8	/* socket name */
#define	MT_ZOMBIE	9	/* zombie proc status */
#define	MT_SOOPTS	10	/* socket options */
#define	MT_FTABLE	11	/* fragment reassembly header */

/* flags to m_get */
#define	M_DONTWAIT	0
#define	M_WAIT		1

/* flags to m_pgalloc */
#define	MPG_MBUFS	0		/* put new mbufs on free list */
#define	MPG_CLUSTERS	1		/* put new clusters on free list */
#define	MPG_SPACE	2		/* don't free; caller wants space */

/* length to m_copy to copy all */
#define	M_COPYALL	1000000000

#define	MGET(m, i, t) \
	{ int ms = splimp(); \
	  if ((m)=mfree) \
		{ if ((m)->m_type != MT_FREE) panic("mget"); (m)->m_type = t; \
		  mbstat.m_mbfree--; mbstat.m_mtypes[t]++; \
		  mfree = (m)->m_next; (m)->m_next = 0; \
		  (m)->m_off = MMINOFF; } \
	  else \
		(m) = m_more(i, t); \
	  (void) splx(ms); }
#define	MFREE(m, n) \
	{ int ms = splimp(); \
	  if ((m)->m_type == MT_FREE) panic("mfree"); \
	  mbstat.m_mtypes[(m)->m_type]--; (m)->m_type = MT_FREE; \
	  if ((m)->m_off > MSIZE) \
		mclput(m); \
	  (n) = (m)->m_next; (m)->m_next = mfree; \
	  (m)->m_off = 0; (m)->m_act = 0; mfree = (m); mbstat.m_mbfree++; \
	  (void) splx(ms); }

/*
 * Mbuf statistics.
 */
struct mbstat {
	short	m_mbufs;	/* mbufs obtained from page pool */
	short	m_mbfree;	/* mbufs on our free list */
	short	m_clusters;	/* clusters obtained from page pool */
	short	m_clfree;	/* free clusters */
	short	m_drops;	/* times failed to find space */
	short	m_mtypes[256];	/* type specific mbuf allocations */
};

#ifdef	KERNEL
extern	struct mbuf mbutl[];		/* virtual address of net free mem */
extern	struct pte Mbmap[];		/* page tables to map mbutl */
struct	mbstat mbstat;
struct	mbuf *mfree, *mclfree;
struct	mbuf *m_get(),*m_getclr(),*m_free(),*m_more(),*m_copy(),*m_pullup();
#define	MCLBYTES	1024		/* size of mbuf clusters */
/* Round bytes up to mclusters */
#define btomcl(x)	(((x) + MCLBYTES - 1) / MCLBYTES)
#endif

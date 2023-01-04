/*	@(#)uipc_mbuf.c 1.1 86/09/25 SMI; from UCB 1.43 83/05/27	*/

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/cmap.h"
#include "../h/map.h"
#include "../h/mbuf.h"
#include "../h/vm.h"
#include "../h/kernel.h"
#include "../h/socketvar.h"
#include "../h/systm.h"

/*
 * The way the code stands, MCLBYTES must be a power of 2
 * and type2 cluster mbufs, if swappable,
 * must swap this size.
 *
 *      Mclbytes was a variable during development, and with the hope
 *      of making it dynamically variable (say, based on the existance
 *      of a Sun-2 Ethernet board); but that hasn't happened.
 */

struct map mbmap[NMBCLUSTERS/4];
char mclrefcnt[NMBCLUSTERS*CLBYTES/MCLBYTES];

/*
 * Map an mcluster address (cast as an mbuf pointer!)
 * to an mcluster index.
 */
#define	mtoclx(x) (((int)x - (int)mbutl) / MCLBYTES)

caddr_t m_clalloc();

/* Allocate the initial mbuf resources */
mbinit()
{
	rminit(mbmap, (long)((NMBCLUSTERS - 1) * CLSIZE), (long)CLSIZE,
	    "mbclusters", NMBCLUSTERS/4);

	if (m_clalloc(btomcl(4096), MPG_MBUFS) == 0)
		goto bad;
	if (m_clalloc(btomcl(4*4096), MPG_CLUSTERS) == 0)
		goto bad;
	return;
bad:
	panic("mbinit");
}

/* Allocate a pool of the requested mbuf type */
caddr_t
m_clalloc(ncl, how)
	register int ncl;
	int how;
{
	int npg, mbx;
	register struct mbuf *m;
	register int i;
	int s;

	npg = btoc(ncl * MCLBYTES);
	ncl = btomcl(npg * NBPG);
	s = splimp();		/* careful: rmalloc isn't reentrant */
	mbx = rmalloc(mbmap, (long)npg);
	(void) splx(s);
	if (mbx == 0)
		return (0);
	if (memall(&Mbmap[mbx], npg, &proc[0], CSYS) == 0) {
		s = splimp();
		rmfree(mbmap, (long)npg, (long)mbx);
		(void) splx(s);
		return (0);
	}
	m = (struct mbuf *)((int)mbutl + ((mbx/CLSIZE) << CLSHIFT));
	vmaccess(&Mbmap[mbx], (caddr_t)m, npg);
	switch (how) {

	case MPG_CLUSTERS:
		s = splimp();
		for (i = 0; i < ncl; i++) {
			m->m_off = 0;
			m->m_next = mclfree;
			mclfree = m;
			m += MCLBYTES / sizeof (*m);
			mbstat.m_clfree++;
		}
		mbstat.m_clusters += ncl;
		(void) splx(s);
		break;

	case MPG_MBUFS:
		for (i = ncl * MCLBYTES / sizeof (*m); i > 0; i--) {
			m->m_off = 0;
			m->m_type = MT_DATA;
			mbstat.m_mtypes[MT_DATA]++;
			mbstat.m_mbufs++;
			(void) m_free(m);
			m++;
		}
		break;

	case MPG_SPACE:		/* used by if_uba */
		break;
	}
	return ((caddr_t)m);
}

#ifdef vax
/*ARGSUSED*/
m_pgfree(addr, n)
	caddr_t addr;
	int n;
{

}
#endif

/* Internal routine to locate more storage for small mbufs */
m_expand()
{

	if (m_clalloc(1, MPG_MBUFS) == 0)
		goto steal;
	return (1);
steal:
	/* should ask protocols to free code */
	return (0);
}

/* NEED SOME WAY TO RELEASE SPACE */

/*
 * Space allocation routines.
 * These are also available as macros
 * for critical paths.
 */

/* Small mbuf allocator */
struct mbuf *
m_get(canwait, type)
	int canwait, type;
{
	register struct mbuf *m;

	MGET(m, canwait, type);
	return (m);
}

/* Small zeroed (cleared) mbuf allocator */
struct mbuf *
m_getclr(canwait, type)
	int canwait, type;
{
	register struct mbuf *m;

	m = m_get(canwait, type);
	if (m == 0)
		return (0);
	bzero(mtod(m, caddr_t), MLEN);
	return (m);
}

/*
 * Generic mbuf unallocator
 * frees the first mbuf in the chain
 * and returns the rest of the chain
 * (compare with m_freem())
 */
struct mbuf *
m_free(m)
	struct mbuf *m;
{
	register struct mbuf *n;

	MFREE(m, n);
	return (n);
}

/* Internal routine called from MGET to locate more small mbufs */
/*ARGSUSED*/
struct mbuf *
m_more(canwait, type)
	int canwait, type;
{
	register struct mbuf *m;

	if (!m_expand()) {
		mbstat.m_drops++;
		return (NULL);
	}
#define m_more(x,y) (panic("m_more"), (struct mbuf *)0)
	MGET(m, canwait, type);
#undef m_more
	return (m);
}

/*
 * Generic mbuf unallocator
 * frees the entire mbuf chain
 * (compare with  m_free())
 */
m_freem(m)
	register struct mbuf *m;
{
	register struct mbuf *n;
	register int s;

	if (m == NULL)
		return;
	s = splimp();
	do {
		MFREE(m, n);
	} while (m = n);
	(void) splx(s);
}

/*
 * Mbuffer utility routines.
 */

/*
 * The m_copy routine create a copy of all, or part, of  a
 * list  of  the mbufs in m0.  Len bytes of data, starting
 * off bytes from the front  of  the  chain,  are  copied.
 * Where  possible,  reference  counts  on  pages are used
 * instead of core to  core  copies.   The  original  mbuf
 * chain  must  have at least off + len bytes of data.  If
 * len is specified as M_COPYALL, all  the  data  present,
 * offset as before, is copied.
 * Warning: MGET is called with M_WAIT.
 */
struct mbuf *
m_copy(m, off, len)
	register struct mbuf *m;
	int off;
	register int len;
{
	register struct mbuf *n, **np;
	struct mbuf *top;
	int type;

	if (len == 0)
		return (0);
	if (off < 0 || len < 0)
		panic("m_copy");
	type = m->m_type;
	while (off > 0) {
		if (m == 0)
			panic("m_copy");
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	np = &top;
	top = 0;
	while (len > 0) {
		if (m == 0) {
			if (len != M_COPYALL)
				panic("m_copy");
			break;
		}
		MGET(n, M_WAIT, type);
		*np = n;
		if (n == 0)
			goto nospace;
		n->m_len = MIN(len, m->m_len - off);
		if (m->m_off > MMAXOFF && n->m_len > MLEN) {
			mcldup(m, n, off);
			n->m_off += off;
		} else
			bcopy(mtod(m, caddr_t)+off, mtod(n, caddr_t),
			    (unsigned)n->m_len);
		if (len != M_COPYALL)
			len -= n->m_len;
		off = 0;
		m = m->m_next;
		np = &n->m_next;
	}
	return (top);
nospace:
	m_freem(top);
	return (0);
}

/*
 * The mbuf chain, n, is appended to the end of m.   Where
 * possible, compaction is performed.
 */
m_cat(m, n)
	register struct mbuf *m, *n;
{

	while (m->m_next)
		m = m->m_next;
	while (n) {
		if (m->m_off >= MMAXOFF ||
		    m->m_off + m->m_len + n->m_len > MMAXOFF) {
			/* just join the two chains */
			m->m_next = n;
			return;
		}
		/* splat the data from one into the other */
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len,
		    (u_int)n->m_len);
		m->m_len += n->m_len;
		n = m_free(n);
	}
}

/*
 * The mbuf chain, m is adjusted in size  by  diff  bytes.
 * If  diff is non-negative, diff bytes are shaved off the
 * front of the mbuf chain.   If  diff  is  negative,  the
 * alteration  is  performed from back to front.  No space
 * is reclaimed in this operation, alterations are  accom-
 * plished  by  changing  the  m_len  and  m_off fields of
 * mbufs.
 */
m_adj(mp, len)
	struct mbuf *mp;
	register int len;
{
	register struct mbuf *m, *n;

	if ((m = mp) == NULL)
		return;
	if (len >= 0) {
		while (m != NULL && len > 0) {
			if (m->m_len <= len) {
				len -= m->m_len;
				m->m_len = 0;
				m = m->m_next;
			} else {
				m->m_len -= len;
				m->m_off += len;
				break;
			}
		}
	} else {
		/* a 2 pass algorithm might be better */
		len = -len;
		while (len > 0 && m->m_len != 0) {
			while (m != NULL && m->m_len != 0) {
				n = m;
				m = m->m_next;
			}
			if (n->m_len <= len) {
				len -= n->m_len;
				n->m_len = 0;
				m = mp;
			} else {
				n->m_len -= len;
				break;
			}
		}
	}
}

/*
 * After a successful call to m_pullup, the  mbuf  at  the
 * head  of the returned list, m, is guaranteed to have at
 * least size bytes of data in contiguous memory (allowing
 * access  via  a pointer, obtained using the mtod macro).
 * If the original data was less than size bytes long, len
 * was  greater  than  the  size of an mbuf data area (112
 * bytes), or required resources were unavailable, m is  0
 * and the original mbuf chain is deallocated.
 */
struct mbuf *
m_pullup(m0, len)
	struct mbuf *m0;
	int len;
{
	register struct mbuf *m, *n;
	int count;

	n = m0;
	if (len > MLEN)
		goto bad;
	MGET(m, M_DONTWAIT, n->m_type);
	if (m == 0)
		goto bad;
	m->m_len = 0;
	do {
		count = MIN(MLEN - m->m_len, len);
		if (count > n->m_len)
			count = n->m_len;
		if (count > 0) {
			bcopy(mtod(n, caddr_t), mtod(m, caddr_t)+m->m_len,
			  (unsigned)count);
			len -= count;
			m->m_len += count;
			n->m_off += count;
			n->m_len -= count;
		}
		if (n->m_len > 0)
			break;
		n = m_free(n);
	} while (n);
	if (len) {
		(void) m_free(m);
		goto bad;
	}
	m->m_next = n;
	return (m);
bad:
	m_freem(n);
	return (0);
}

/*
 * Copy an mbuf to the contiguous area pointed to by cp.
 * Skip <off> bytes and copy <len> bytes.
 * Returns the number of bytes not transferred.
 * The mbuf is NOT changed.
 */
int
m_cpytoc(m, off, len, cp)
	register struct mbuf *m;
	register int off, len;
	register caddr_t cp;
{
	register int ml;

	if (m == NULL || off < 0 || len < 0 || cp == NULL)
		panic("m_cpytoc");
	while (off && m)
		if (m->m_len <= off) {
			off -= m->m_len;
			m = m->m_next;
			continue;
		} else
			break;
	if (m == NULL)
		return (len);

	ml = imin(len, m->m_len - off);
	bcopy(mtod(m, caddr_t)+off, cp, (u_int)ml);
	cp += ml;
	len -= ml;
	m = m->m_next;

	while (len && m) {
		ml = m->m_len;
		bcopy(mtod(m, caddr_t), cp, (u_int)ml);
		cp += ml;
		len -= ml;
		m = m->m_next;
	}

	return (len);
}

/*
 * Append a contiguous hunk of memory to a particular mbuf;
 * that is, it does not follow the mbuf chain.
 * XXX It should return 1 if it won't fit.  Maybe someday.
 */
int
m_cappend(cp, len, m)
	char *cp;
	register int len;
	register struct mbuf *m;
{
	bcopy(cp, mtod(m, char *) + m->m_len, (u_int)len);
	m->m_len += len;
	return (0);
}

/*
 * Tally the bytes used in an mbuf chain.
 * op: -1 == subtract; 0 == assign; 1 == add;
 */
m_tally(m, op, cc, mbcnt)
	register struct mbuf *m;
	int op, *cc, *mbcnt;
{
	struct sockbuf sockbuf;
	register struct sockbuf *sb = &sockbuf;

	bzero((caddr_t)sb, sizeof(*sb));
	while (m) {
		sballoc(sb, m);
		m = m->m_next;
	}
	if (cc)
		switch (op) {
		case -1:
			*cc -= sb->sb_cc;
			break;
		case 0:
			*cc  = sb->sb_cc;
			break;
		case 1:
			*cc += sb->sb_cc;
			break;
		}
	if (mbcnt)
		switch (op) {
		case -1:
			*mbcnt -= sb->sb_mbcnt;
			break;
		case 0:
			*mbcnt  = sb->sb_mbcnt;
			break;
		case 1:
			*mbcnt += sb->sb_mbcnt;
			break;
		}
}

/*
 * Given an mbuf, allocate and attach a cluster mbuf to it.
 * Return 1 if successful, 0 otherwise.
 * NOTE: m->m_len is set to MCLBYTES!
 */
mclget(m)
	register struct mbuf *m;
{
	int ms;
	register struct mbuf *p;

	ms = splimp();
	if (mclfree == 0)
		/* XXX need a way to  reclaim */
		(void) m_clalloc(1, MPG_CLUSTERS);
	if (p = mclfree) {
		++mclrefcnt[mtoclx(p)];
		mbstat.m_clfree--;
		mclfree = p->m_next;
		m->m_len = MCLBYTES;
		m->m_off = (int)p - (int)m;
		m->m_cltype = 1;
	}
	(void) splx(ms);
	return (p ? 1 : 0);
}

/* Allocate a "funny" mbuf, that is, one whose data is owned by someone else */
struct mbuf *
mclgetx(fun, arg, addr, len, wait)
	int (*fun)(), arg, len, wait;
	caddr_t addr;
{
	register struct mbuf *m;

	MGET(m, wait, MT_DATA);
	if (m == 0)
		return (0);
	m->m_off = (int)addr - (int)m;
	m->m_len = len;
	m->m_cltype = 2;
	m->m_clfun = fun;
	m->m_clarg = arg;
	m->m_clswp = NULL;
	return (m);
}

/* Generic cluster mbuf unallocator -- invoked from within MFREE */
mclput(m)
	register struct mbuf *m;
{

	switch (m->m_cltype) {
	case 1:
		m = (struct mbuf *)(mtod(m, int) & ~(MCLBYTES - 1));
		if (--mclrefcnt[mtoclx(m)] == 0) {
			m->m_next = mclfree;
			mclfree = m;
			mbstat.m_clfree++;
		}
		break;

	case 2:
		(*m->m_clfun)(m->m_clarg);
		break;

	default:
		panic("mclput");
	}
}

/* Internal routine used for mcldup on funny mbufs */
static
buffree(arg)
	int arg;
{
	extern int kmem_free_intr();

	kmem_free_intr((caddr_t)arg, *(u_int *)arg);
}

/*
 * Generic cluster mbuf duplicator
 * which duplicates <m> into <n>.
 * If <m> is a regular cluster mbuf, mcldup simply
 * bumps the reference count and ignores <off>.
 * If <m> is a funny mbuf, mcldup allocates a chunck
 * kernel memory and makes a copy, starting at <off>.
 * XXX does not set the m_len field in <n>!
 */
mcldup(m, n, off)
	register struct mbuf *m, *n;
	int off;
{
	extern caddr_t kmem_alloc();
	register struct mbuf *p;
	register caddr_t copybuf;

	switch (m->m_cltype) {
	case 1:
		p = mtod(m, struct mbuf *);
		n->m_off = (int)p - (int)n;
		n->m_cltype = 1;
		mclrefcnt[mtoclx(p)]++;
		break;
	case 2:
		copybuf = kmem_alloc((u_int)(n->m_len + sizeof (int)));
		* (int *) copybuf = n->m_len + sizeof (int);
		bcopy(mtod(m, caddr_t) + off, copybuf + sizeof (int),
		    (u_int)n->m_len);
		n->m_off = (int)copybuf + sizeof (int) - (int)n - off;
		n->m_cltype = 2;
		n->m_clfun = buffree;
		n->m_clarg = (int)copybuf;
		n->m_clswp = NULL;
		break;
	default:
		panic("mcldup");
	}
}

/*
 * Move an mbuf chain to contiguous locations.
 * Checks for possibility of page exchange to accomplish move.
 * Free chain when moved.
 */
m_movtoc(m, to, count)
	register struct mbuf *m;
	register caddr_t to;
	register int count;
{
	register struct mbuf *m0;
	register caddr_t from;
	register int i;

	while (m != NULL) {
		i = MIN(m->m_len, count);
		from = mtod(m, caddr_t);
		if (i >= MCLBYTES && m->m_cltype == 2 && m->m_clswp &&
		    (((int)from | (int)to) & (MCLBYTES-1)) == 0 &&
		    (*m->m_clswp)(m->m_clarg, from, to)) {
			i -= MCLBYTES;
			from += MCLBYTES;
			to += MCLBYTES;
		}
		if (i > 0) {
			bcopy(from, to, (unsigned)i);
			count -= i;
			to += i;
		}
		m0 = m;
		MFREE(m0, m);
	}
	return (count);
}

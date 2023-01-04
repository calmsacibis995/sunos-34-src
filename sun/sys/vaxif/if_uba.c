#ifndef lint
static	char sccsid[] = "@(#)if_uba.c 1.1 86/09/25 SMI"; /* from UCB 6.1 83/07/29 */
#endif

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/cmap.h"
#include "../h/vmmac.h"
#include "../h/socket.h"

#include "../net/if.h"

#include "../vax/mtpr.h"
#include "../vaxif/if_uba.h"
#include "../vaxuba/ubareg.h"
#include "../vaxuba/ubavar.h"

#define	mtocl(x) (((int)x - (int)mbutl) / mclbytes)	/* XXX */

/*
 * Routines supporting UNIBUS network interfaces.
 *
 * TODO:
 *	Support interfaces using only one BDP statically.
 */

/*
 * Init UNIBUS for interface on uban whose headers of size hlen are to
 * end on a page boundary.  We allocate a UNIBUS map register for the page
 * with the header, and nmr more UNIBUS map registers for i/o on the adapter,
 * doing this twice: once for reading and once for writing.  We also
 * allocate page frames in the mbuffer pool for these pages.
 */
if_ubainit(ifu, uban, hlen, nmr)
	register struct ifuba *ifu;
	int uban, hlen, nmr;
{
	register caddr_t cp;
	int i, ncl;

	ncl = clrnd(nmr + CLSIZE) / CLSIZE;
	if (ifu->ifu_r.ifrw_addr)
		cp = ifu->ifu_r.ifrw_addr - (CLBYTES - hlen);
	else {
		cp = m_clalloc(2 * ncl, MPG_SPACE);
		if (cp == 0)
			return (0);
		ifu->ifu_r.ifrw_addr = cp + CLBYTES - hlen;
		ifu->ifu_w.ifrw_addr = ifu->ifu_r.ifrw_addr + ncl * CLBYTES;
		ifu->ifu_hlen = hlen;
		ifu->ifu_uban = uban;
		ifu->ifu_uba = uba_hd[uban].uh_uba;
	}
	if (if_ubaalloc(ifu, &ifu->ifu_r, nmr) == 0)
		goto bad;
	if (if_ubaalloc(ifu, &ifu->ifu_w, nmr) == 0)
		goto bad2;
	for (i = 0; i < nmr; i++)
		ifu->ifu_wmap[i] = ifu->ifu_w.ifrw_mr[i];
	ifu->ifu_xswapd = 0;
	return (1);
bad2:
	ubarelse(ifu->ifu_uban, &ifu->ifu_r.ifrw_info);
bad:
	m_pgfree(cp, 2 * ncl);
	ifu->ifu_r.ifrw_addr = 0;
	return (0);
}

/*
 * Setup either a ifrw structure by allocating UNIBUS map registers,
 * possibly a buffered data path, and initializing the fields of
 * the ifrw structure to minimize run-time overhead.
 */
static
if_ubaalloc(ifu, ifrw, nmr)
	struct ifuba *ifu;
	register struct ifrw *ifrw;
	int nmr;
{
	register int info;

	info =
	    uballoc(ifu->ifu_uban, ifrw->ifrw_addr, nmr*NBPG + ifu->ifu_hlen,
	        ifu->ifu_flags);
	if (info == 0)
		return (0);
	ifrw->ifrw_info = info;
	ifrw->ifrw_bdp = UBAI_BDP(info);
	ifrw->ifrw_proto = UBAMR_MRV | (UBAI_BDP(info) << UBAMR_DPSHIFT);
	ifrw->ifrw_mr = &ifu->ifu_uba->uba_map[UBAI_MR(info) + 1];
	return (1);
}

/*
 * Pull read data off a interface.
 * Len is length of data, with local net header stripped.
 * Off is non-zero if a trailer protocol was used, and
 * gives the offset of the trailer information.
 * We copy the trailer information and then all the normal
 * data into mbufs.  When full cluster sized units are present
 * on the interface on cluster boundaries we can get them more
 * easily by remapping, and take advantage of this here.
 */
struct mbuf *
if_rubaget(ifu, totlen, off0)
	register struct ifuba *ifu;
	int totlen, off0;
{
	struct mbuf *top, **mp, *m;
	int off = off0, len;
	register caddr_t cp = ifu->ifu_r.ifrw_addr + ifu->ifu_hlen;


	top = 0;
	mp = &top;
	while (totlen > 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0)
			goto bad;
		if (off) {
			len = totlen - off;
			cp = ifu->ifu_r.ifrw_addr + ifu->ifu_hlen + off;
		} else
			len = totlen;
		if (len >= CLBYTES) {
			struct mbuf *p;
			struct pte *cpte, *ppte;
			int x, *ip, i;

			if (mclget(m) == 0)
				goto nopage;
			p = mtod(m, struct mbuf *);
			if (!claligned(cp))
				goto copy;

			/*
			 * Switch pages mapped to UNIBUS with new page p,
			 * as quick form of copy.  Remap UNIBUS and invalidate.
			 */
			cpte = &Mbmap[mtocl(cp)*CLSIZE];
			ppte = &Mbmap[mtocl(p)*CLSIZE];
			x = btop(cp - ifu->ifu_r.ifrw_addr);
			ip = (int *)&ifu->ifu_r.ifrw_mr[x];
			for (i = 0; i < CLSIZE; i++) {
				struct pte t;
				t = *ppte; *ppte++ = *cpte; *cpte = t;
				*ip++ =
				    cpte++->pg_pfnum|ifu->ifu_r.ifrw_proto;
				mtpr(TBIS, cp);
				cp += NBPG;
				mtpr(TBIS, (caddr_t)p);
				p += NBPG / sizeof (*p);
			}
			goto nocopy;
		}
nopage:
		m->m_len = MIN(MLEN, len);
		m->m_off = MMINOFF;
copy:
		bcopy(cp, mtod(m, caddr_t), (unsigned)m->m_len);
		cp += m->m_len;
nocopy:
		*mp = m;
		mp = &m->m_next;
		if (off) {
			/* sort of an ALGOL-W style for statement... */
			off += m->m_len;
			if (off == totlen) {
				cp = ifu->ifu_r.ifrw_addr + ifu->ifu_hlen;
				off = 0;
				totlen = off0;
			}
		} else
			totlen -= m->m_len;
	}
	return (top);
bad:
	m_freem(top);
	return (0);
}

/*
 * Map a chain of mbufs onto a network interface
 * in preparation for an i/o operation.
 * The argument chain of mbufs includes the local network
 * header which is copied to be in the mapped, aligned
 * i/o space.
 */
if_wubaput(ifu, m)
	register struct ifuba *ifu;
	register struct mbuf *m;
{
	register struct mbuf *mp;
	register caddr_t cp, dp;
	register int i;
	int xswapd = 0;
	int x, cc;

	cp = ifu->ifu_w.ifrw_addr;
	while (m) {
		dp = mtod(m, char *);
		if (claligned(cp) && claligned(dp) && m->m_len == CLBYTES) {
			struct pte *pte; int *ip;
			pte = &Mbmap[mtocl(dp)*CLSIZE];
			x = btop(cp - ifu->ifu_w.ifrw_addr);
			ip = (int *)&ifu->ifu_w.ifrw_mr[x];
			for (i = 0; i < CLSIZE; i++)
				*ip++ =
				    ifu->ifu_w.ifrw_proto | pte++->pg_pfnum;
			xswapd |= 1 << (x>>(CLSHIFT-PGSHIFT));
			mp = m->m_next;
			m->m_next = ifu->ifu_xtofree;
			ifu->ifu_xtofree = m;
			cp += m->m_len;
		} else {
			bcopy(mtod(m, caddr_t), cp, (unsigned)m->m_len);
			cp += m->m_len;
			MFREE(m, mp);
		}
		m = mp;
	}

	/*
	 * Xswapd is the set of clusters we just mapped out.  Ifu->ifu_xswapd
	 * is the set of clusters mapped out from before.  We compute
	 * the number of clusters involved in this operation in x.
	 * Clusters mapped out before and involved in this operation
	 * should be unmapped so original pages will be accessed by the device.
	 */
	cc = cp - ifu->ifu_w.ifrw_addr;
	x = ((cc - ifu->ifu_hlen) + CLBYTES - 1) >> CLSHIFT;
	ifu->ifu_xswapd &= ~xswapd;
	xswapd &= ~ifu->ifu_xswapd;
	while (i = ffs(ifu->ifu_xswapd)) {
		i--;
		if (i >= x)
			break;
		ifu->ifu_xswapd &= ~(1<<i);
		i *= CLSIZE;
		for (x = 0; x < CLSIZE; x++) {
			ifu->ifu_w.ifrw_mr[i] = ifu->ifu_wmap[i];
			i++;
		}
	}
	ifu->ifu_xswapd |= xswapd;
	return (cc);
}

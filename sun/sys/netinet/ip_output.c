/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ip_output.c 1.5 87/02/16 SMI; from UCB 7.1 6/5/86
 */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/errno.h"
#include "../h/socket.h"
#include "../h/socketvar.h"

#include "../net/if.h"
#include "../net/route.h"

#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"

#ifdef vax
#include "../vax/mtpr.h"
#endif

ip_output(m, opt, ro, flags)
	struct mbuf *m;
	struct mbuf *opt;
	struct route *ro;
	int flags;
{
	register struct ip *ip = mtod(m, struct ip *);
	register struct ifnet *ifp;
	int len, hlen = sizeof (struct ip), off, error = 0;
	struct route iproute;
	struct sockaddr_in *dst;
	extern struct ifnet loif;

	if (opt)				/* XXX */
		(void) m_free(opt);		/* XXX */
	/*
	 * Fill in IP header.
	 */
	ip->ip_hl = hlen >> 2;
	if ((flags & IP_FORWARDING) == 0) {
		ip->ip_v = IPVERSION;
		ip->ip_off &= IP_DF;
		ip->ip_id = htons(ip_id++);
	}

	/*
	 * Route packet.
	 */
	if (ro == 0) {
		ro = &iproute;
		bzero((caddr_t)ro, sizeof (*ro));
	}
	dst = (struct sockaddr_in *)&ro->ro_dst;
	/*
	 * If there is a cached route,
	 * check that it is to the same destination
	 * and is still up.  If not, free it and try again.
	 */
	if (ro->ro_rt && ((ro->ro_rt->rt_flags & RTF_UP) == 0 ||
	   dst->sin_addr.s_addr != ip->ip_dst.s_addr)) {
		RTFREE(ro->ro_rt);
		ro->ro_rt = (struct rtentry *)0;
	}
	if (ro->ro_rt == 0) {
		dst->sin_family = AF_INET;
		dst->sin_addr = ip->ip_dst;
	}
	/*
	 * If routing to interface only,
	 * short circuit routing lookup.
	 */
	if (flags & IP_ROUTETOIF) {
#include "nd.h"
#if NND > 0
			/* XXX disgusting kludge ... should use ro argument */
			if (ip->ip_dst.s_net == 0) {
				extern struct ifnet *ndif;

				ifp = ndif;
			} else
#endif
				ifp = if_ifwithdstaddr((struct sockaddr *)dst);
			if (ifp == 0)
				ifp = if_ifwithnet((struct sockaddr *)dst);
			if (ifp == 0)
				ifp = if_ifonnetof(in_netof(dst->sin_addr));
			if (ifp != 0) {
				goto gotif;
			}
	}
	   /*
	    * Fall back on normal routing if ROUTETOIF fails.
	    */
		if (ro->ro_rt == 0)
			rtalloc(ro);
		if (ro->ro_rt == 0 || (ifp = ro->ro_rt->rt_ifp) == 0) {
			error = ENETUNREACH;
			goto bad;
		}
		ro->ro_rt->rt_use++;
		if (ro->ro_rt->rt_flags & (RTF_GATEWAY|RTF_HOST))
			dst = (struct sockaddr_in *)&ro->ro_rt->rt_gateway;
gotif:
#ifndef notdef
	/*
	 * If source address not specified yet, use address
	 * of outgoing interface.
	 */
	if (in_lnaof(ip->ip_src) == INADDR_ANY)
		ip->ip_src.s_addr =
		    ((struct sockaddr_in *)&ifp->if_addr)->sin_addr.s_addr;
#endif

	/*
	 * Look for broadcast address and
	 * and verify user is allowed to send
	 * such a packet.
	 */
	if (in_broadcast(dst->sin_addr)) {
		if ((ifp->if_flags & IFF_BROADCAST) == 0) {
			error = EADDRNOTAVAIL;
			goto bad;
		}
#ifdef notdef
		if ((flags & IP_ALLOWBROADCAST) == 0) {
			error = EACCES;
			goto bad;
		}
#endif
		/* don't allow broadcast messages to be fragmented */
		if (ip->ip_len > ifp->if_mtu) {
			error = EMSGSIZE;
			goto bad;
		}
	}

	/*
	 * If small enough for interface, can just send directly.
	 */
	if (ip->ip_len <= ifp->if_mtu) {
		ip->ip_len = htons((u_short)ip->ip_len);
		ip->ip_off = htons((u_short)ip->ip_off);
		ip->ip_sum = 0;
		ip->ip_sum = in_cksum(m, hlen);
		error = (*ifp->if_output)(ifp, m, (struct sockaddr *)dst);
		goto done;
	}

	/*
	 * Too large for interface; fragment if possible.
	 * Must be able to put at least 8 bytes per fragment.
	 */
	if (ip->ip_off & IP_DF) {
		error = EMSGSIZE;
		goto bad;
	}
	len = (ifp->if_mtu - hlen) &~ 7;
	if (len < 8) {
		error = EMSGSIZE;
		goto bad;
	}
	if (hlen == sizeof (struct ip) &&
	    ip_frag2(m, ip, len, &error, ifp, (struct sockaddr *)dst))
		goto done;

	/*
	 * Discard IP header from logical mbuf for m_copy's sake.
	 * Loop through length of segment, make a copy of each
	 * part and output.
	 */
	m->m_len -= sizeof (struct ip);
	m->m_off += sizeof (struct ip);
	for (off = 0; off < ip->ip_len-hlen; off += len) {
		struct mbuf *mh = m_get(M_DONTWAIT, MT_HEADER);
		struct ip *mhip;

		if (mh == 0) {
			error = ENOBUFS;
			goto bad;
		}
		mh->m_off = MMAXOFF - hlen;
		mhip = mtod(mh, struct ip *);
		*mhip = *ip;
		if (hlen > sizeof (struct ip)) {
			int olen = ip_optcopy(ip, mhip, off);
			mh->m_len = sizeof (struct ip) + olen;
		} else
			mh->m_len = sizeof (struct ip);
		mhip->ip_off = (off >> 3) + (ip->ip_off & ~(IP_MF|IP_DF));

		/*
		 * If the packet we're fragmenting has more fragments from
  		 * other systems, propagate the MORE_FRAGMENTS flag.
		 */
		if(ip->ip_off & IP_MF) mhip->ip_off |= IP_MF;

		if (off + len >= ip->ip_len-hlen)
			len = mhip->ip_len = ip->ip_len - hlen - off;
		else {
			mhip->ip_len = len;
			mhip->ip_off |= IP_MF;
		}
		mhip->ip_len += sizeof (struct ip);
		mhip->ip_len = htons((u_short)mhip->ip_len);
		mh->m_next = m_copy(m, off, len);
		if (mh->m_next == 0) {
			(void) m_free(mh);
			error = ENOBUFS;	/* ??? */
			goto bad;
		}
		mhip->ip_off = htons((u_short)mhip->ip_off);
		mhip->ip_sum = 0;
		mhip->ip_sum = in_cksum(mh, hlen);
		if (error = (*ifp->if_output)(ifp, mh, (struct sockaddr *)dst))
			break;
	}
	m_freem(m);
	goto done;

bad:
	m_freem(m);
done:
	if (ro == &iproute && (flags & IP_ROUTETOIF) == 0 && ro->ro_rt)
		RTFREE(ro->ro_rt);
	return (error);
}

/*
 * Copy options from ip to jp.
 * If off is 0 all options are copied
 * otherwise copy selectively.
 */
ip_optcopy(ip, jp, off)
	struct ip *ip, *jp;
	int off;
{
	register u_char *cp, *dp;
	int opt, optlen, cnt;

	cp = (u_char *)(ip + 1);
	dp = (u_char *)(jp + 1);
	cnt = (ip->ip_hl << 2) - sizeof (struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else
			optlen = cp[1];
		if (optlen > cnt)			/* XXX */
			optlen = cnt;			/* XXX */
		if (off == 0 || IPOPT_COPIED(opt)) {
			bcopy((caddr_t)cp, (caddr_t)dp, (unsigned)optlen);
			dp += optlen;
		}
	}
	for (optlen = dp - (u_char *)(jp+1); optlen & 0x3; optlen++)
		*dp++ = IPOPT_EOL;
	return (optlen);
}

/*
 * Attempt to fragment type 2 mbug chain.
 * Works only if each mbuf is smaller than a packet.
 * This saves copying all the data.
 */
ip_frag2(m, ip, maxpacketlen, errorp, ifp, dst)
	register struct mbuf *m;
	register struct ip *ip;
	register int maxpacketlen;
	int *errorp;
	struct ifnet *ifp;
	struct sockaddr *dst;
{
	struct mbuf *mm;
	struct mbuf *lastm;
	register struct mbuf *mh;
	register int fraglen, fragoff, pktlen, n;
	struct ip *nextip;

	/*
	 * Check whether we can do it.
	 */
	mm = m;
	n = 0;
	while (m) {
		if (m->m_type == 2) {
			n++;
		}
		if (m->m_len + sizeof (struct ip) > maxpacketlen) {
			return (0);
		}
		m = m->m_next;
	}
	if (n == 0) {	/* higher level does type 1 chain better */
		return (0);
	}
	m = mm;
	fragoff = 0;
	while (m) {
		pktlen = 0;
		mm = m;
		/*
		 * Gather up all the mbufs that will fit in a frag.
		 */
		while (m && pktlen + m->m_len <= maxpacketlen) {
			pktlen += m->m_len;
			lastm = m;
			m = m->m_next;
		}
		fraglen = pktlen - sizeof (struct ip);
		lastm->m_next = 0;
		if (m) {
			/*
			 * There are more frags, so we prepend
			 * a copy of the ip hdr to the rest
			 * of the chain.
			 */
			MGET(mh, M_DONTWAIT, MT_HEADER);
			if (mh == 0) {
				*errorp = ENOBUFS;
				break;
			}
			mh->m_off = MMAXOFF - sizeof (struct ip) - 8;
			nextip = mtod(mh, struct ip *);
			/* copy the ip header */
			*nextip = *ip;
			mh->m_len = sizeof (struct ip);
			mh->m_next = m;
			m = mh;
			if (n = (fraglen & 7)) {
				/*
				 * IP fragments must be a multiple of
				 * 8 bytes long so we must play games.
				 */
				bcopy(mtod(lastm, caddr_t) + lastm->m_len - n,
					(caddr_t) (nextip + 1), (unsigned) n);
				lastm->m_len -= n;
				mh->m_len += n;
				pktlen -= n;
				fraglen -= n;
			}
			ip->ip_off = htons((u_short) ((fragoff >> 3) | IP_MF));
		} else {
			ip->ip_off = htons((u_short) (fragoff >> 3));
		}
		/*
		 * Fix up the ip header for the mm chain and send it off.
		 */
		if (ip->ip_len < pktlen) {
			ip->ip_len = htons((u_short) ip->ip_len);
			if (m) {
				m_freem(m);
				m = 0;
			}
		} else {
			ip->ip_len = htons((u_short) pktlen);
			if (m) {
				nextip->ip_len -= fraglen;
			}
		}
		ip->ip_sum = 0;
		ip->ip_sum = in_cksum(mm, sizeof (struct ip));
		if (*errorp = (*ifp->if_output)(ifp, mm, dst)) {
			break;
		}
		ip = nextip;
		fragoff += fraglen;
	}
	return (1);
}

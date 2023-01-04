/*	@(#)raw_cb.c 1.1 86/09/25 SMI; from UCB 4.20 83/06/30	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../h/protosw.h"
#include "../h/ioctl.h"
#include "../h/time.h"

#include "../net/if.h"
#include "../net/route.h"
#include "../net/raw_cb.h"
#include "../net/nit.h"
#include "../netinet/in.h"
#include "../netpup/pup.h"

#ifdef vax
#include "../vax/mtpr.h"
#endif

/*
 * Routines to manage the raw protocol control blocks. 
 *
 * TODO:
 *	hash lookups by protocol family/protocol + address family
 *	take care of unique address problems per AF?
 *	redo address binding to allow wildcards
 */

/*
 * Allocate a control block and a nominal amount
 * of buffer space for the socket.
 */
raw_attach(so)
	register struct socket *so;
{
	struct mbuf *m;
	register struct rawcb *rp;

	m = m_getclr(M_DONTWAIT, MT_PCB);
	if (m == 0)
		return (ENOBUFS);
	if (sbreserve(&so->so_snd, RAWSNDQ) == 0)
		goto bad;
	/* recvfrom needs extra space for address */
	if (sbreserve(&so->so_rcv, RAWRCVQ + sizeof(struct sockaddr)) == 0)
		goto bad2;
	rp = mtod(m, struct rawcb *);
	rp->rcb_socket = so;
	insque(rp, &rawcb);
	so->so_pcb = (caddr_t)rp;
	rp->rcb_pcb = 0;
	return (0);
bad2:
	sbrelease(&so->so_snd);
bad:
	(void) m_free(m);
	return (ENOBUFS);
}

/*
 * Detach the raw connection block and discard
 * socket resources.
 */
raw_detach(rp)
	register struct rawcb *rp;
{
	struct socket *so = rp->rcb_socket;

	so->so_pcb = 0;
	sofree(so);
	remque(rp);
	m_freem(dtom(rp));
}

/*
 * Disconnect and possibly release resources.
 */
raw_disconnect(rp)
	struct rawcb *rp;
{

	rp->rcb_flags &= ~RAW_FADDR;
	if (rp->rcb_socket->so_state & SS_NOFDREF)
		raw_detach(rp);
}

raw_bind(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	struct sockaddr *addr = mtod(nam, struct sockaddr *);
	register struct rawcb *rp;

	if (ifnet == 0)
		return (EADDRNOTAVAIL);
/* BEGIN DUBIOUS */
	/*
	 * Should we verify address not already in use?
	 * Some say yes, others no.
	 */
	switch (addr->sa_family) {

#ifdef NIT
	case AF_NIT:
		if (nit_ifwithaddr(addr) == 0)
			return (EADDRNOTAVAIL);
		break;
#endif NIT

#ifdef INET
	case AF_IMPLINK:
	case AF_INET: {
		if (((struct sockaddr_in *)addr)->sin_addr.s_addr &&
		    if_ifwithaddr(addr) == 0)
			return (EADDRNOTAVAIL);
		break;
	}
#endif

#ifdef PUP
	/*
	 * Curious, we convert PUP address format to internet
	 * to allow us to verify we're asking for an Ethernet
	 * interface.  This is wrong, but things are heavily
	 * oriented towards the internet addressing scheme, and
	 * converting internet to PUP would be very expensive.
	 */
	case AF_PUP: {
		struct sockaddr_pup *spup = (struct sockaddr_pup *)addr;
		struct sockaddr_in inpup;

		bzero((caddr_t)&inpup, (unsigned)sizeof(inpup));
		inpup.sin_family = AF_INET;
		inpup.sin_addr = if_makeaddr((int)spup->spup_net,
		    (int)spup->spup_host);
		if (inpup.sin_addr.s_addr &&
		    if_ifwithaddr((struct sockaddr *)&inpup) == 0)
			return (EADDRNOTAVAIL);
		break;
	}
#endif

	default:
		return (EAFNOSUPPORT);
	}
/* END DUBIOUS */
	rp = sotorawcb(so);
	bcopy((caddr_t)addr, (caddr_t)&rp->rcb_laddr, sizeof (*addr));
	rp->rcb_flags |= RAW_LADDR;

	return (0);
}

/*
 * Associate a peer's address with a
 * raw connection block.
 */
raw_connaddr(rp, nam)
	struct rawcb *rp;
	struct mbuf *nam;
{
	struct sockaddr *addr = mtod(nam, struct sockaddr *);

	bcopy((caddr_t)addr, (caddr_t)&rp->rcb_faddr, sizeof(*addr));
	rp->rcb_flags |= RAW_FADDR;
}

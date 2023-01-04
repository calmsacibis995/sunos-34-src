#ifndef lint
static  char sccsid[] = "@(#)if_subr.c 1.3 87/02/19 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Device independent subroutines used by Ethernet device drivers.
 * Mostly this code takes care of protocols and sockets and
 * stuff.  Knows about Ethernet in general, but is ignorant of the
 * details of any particular Ethernet chip or board.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/socket.h"
#include "../h/errno.h"
#include "../h/time.h"

#include "../net/if.h"
#include "../net/route.h"
#include "../net/netisr.h"
#ifdef NIT
#include "../net/nit.h"
#endif
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/if_ether.h"

extern	struct ifnet loif;

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 * Any hardware-specific initialization is done by the caller.
 */
ether_attach(ifp, unit, name, init, ioctl, output, reset)
	register struct ifnet *ifp;
	int unit;
	char *name;
	int (*init)(), (*ioctl)(), (*output)(), (*reset)();
{

	ifp->if_unit = unit;
	ifp->if_name = name;
	ifp->if_mtu = ETHERMTU;

	((struct sockaddr_in *)&(ifp->if_addr))->sin_family = AF_INET;
	ifp->if_init = init;
	ifp->if_ioctl = ioctl;
	ifp->if_output = output;
	ifp->if_reset = reset;
	if_attach(ifp);
}

address_known(ifp)
	struct ifnet *ifp;
{

	return (((struct sockaddr_in *)&(ifp->if_addr))->sin_addr.s_addr != 0);
}

/*
 * This is used from within an ioctl service routine to process the
 * SIOCSIFADDR request.  This should be called at interrupt priority
 * level to prevent races.
 */
set_if_addr(ifp, data, enaddr)
	register struct ifnet *ifp;
	caddr_t data;
	struct ether_addr *enaddr;
{
	struct sockaddr_in *sin = (struct sockaddr_in *)data;
	struct sockaddr *sa = (struct sockaddr *)data;

	if (sa->sa_family == AF_UNSPEC) {
		if (sa->sa_data[0] & 1)  /* broad or multi-cast */
			return (EINVAL);

		*enaddr = *(struct ether_addr *)sa->sa_data;
		(*ifp->if_init)(ifp->if_unit);
		return (0);
	}

	if (sin->sin_family != AF_INET)
		return (EINVAL);

	if (ifp->if_flags & IFF_RUNNING)
		if_rtinit(ifp, -1);	/* delete previous route */
	setaddr(ifp, sin);
	(*ifp->if_init)(ifp->if_unit);

	return (0);
}

/*
 * Ethernet output routine.
 * Encapsulate a packet of type family for the local net.
 * Use trailer local net encapsulation if enough data in first
 * packet leaves a multiple of 512 bytes of data in remainder.
 * If destination is this address or broadcast, send packet to
 * loop device to kludge around the fact that interfaces can't
 * talk to themselves.
 */
ether_output(ifp, m0, dst, startout, ap)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
	int (*startout)(); /* Hardware-specific routine to start output */
	struct arpcom *ap;
{
	int type, s;
	struct ether_addr edst;
	struct in_addr idst;
	register struct mbuf *m = m0;
	register struct ether_header *header;
	register int off = 0;
	struct mbuf *mcopy = (struct mbuf *)0;		/* Null */

	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
		return (ENETDOWN);

	switch (dst->sa_family) {

#ifdef INET
	case AF_INET:
		idst = ((struct sockaddr_in *)dst)->sin_addr;
		if (!arpresolve(ap, m, &idst, &edst))
			return (0);	/* if not yet resolved */
		if (bcmp((caddr_t)&edst, (caddr_t)&etherbroadcastaddr,
	 		sizeof (struct ether_addr)) == 0)
			mcopy = m_copy(m, 0, M_COPYALL);
		off = ((u_short)mtod(m, struct ip *)->ip_len) - m->m_len;
		if ((ifp->if_flags & IFF_NOTRAILERS) == 0 &&
		    off > 0 && (off & 0x1ff) == 0 &&
		    m->m_off >= MMINOFF + 2 * sizeof (u_short)) {
			type = ETHERPUP_TRAIL + (off>>9);
			m->m_off -= 2 * sizeof (u_short);
			m->m_len += 2 * sizeof (u_short);
			*mtod(m, u_short *) = ETHERPUP_IPTYPE;
			*(mtod(m, u_short *) + 1) = m->m_len;
		} else {
			type = ETHERPUP_IPTYPE;
			off = 0;
		}
		break;
#endif INET

	case AF_UNSPEC:
		header = (struct ether_header *)dst->sa_data;
		edst = header->ether_dhost;
		type = header->ether_type;
		break;

	default:
		identify(ifp);
		printf("can't handle AF 0x%x\n", dst->sa_family);
		m_freem(m0);
		return (EAFNOSUPPORT);
	}

	if (off) {
		/*
		 * Packet to be sent as trailer: move first packet
		 * (control information) to end of chain.
		 */
		while (m->m_next)
			m = m->m_next;
		m->m_next = m0;
		m = m0->m_next;
		m0->m_next = 0;
		m0 = m;
	}

	/*
	 * Add local net header.  If no space in first mbuf,
	 * allocate another.
	 */
	if (m->m_off > MMAXOFF ||
	    MMINOFF + sizeof (struct ether_header) > m->m_off) {
		m = m_get(M_DONTWAIT, MT_DATA);
#ifdef DEBUG
		chkmbuf(m);
#endif DEBUG
		if (m == 0) {
			m_freem(m0);
			identify(ifp);
			printf("WARNING: no mbufs\n");
			return (ENOBUFS);
		}
		m->m_next = m0;
		m->m_len = sizeof (struct ether_header);
	} else {
		m->m_off -= sizeof (struct ether_header);
		m->m_len += sizeof (struct ether_header);
	}
	header = mtod(m, struct ether_header *);
	header->ether_dhost = edst;
	header->ether_type = type;

	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	s = splimp();
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
#ifdef DEBUG
		chkmbuf(m);
#endif DEBUG
		m_freem(m);
		identify(ifp);
		printf("WARNING: if_snd full\n");
		(void) splx(s);
		return (ENOBUFS);
	}
	IF_ENQUEUE(&ifp->if_snd, m);
	(*startout)(ifp->if_unit);
	(void) splx(s);
	return (mcopy ? looutput(&loif, mcopy, dst) : 0);
}

/*
 * This is called from within set_if_addr and is not used by external
 * routines.
 */
setaddr(ifp, sin)
	register struct ifnet *ifp;
	register struct sockaddr_in *sin;
{

	ifp->if_addr = *(struct sockaddr *)sin;
	ifp->if_net = in_netof(sin->sin_addr);
	ifp->if_host[0] = in_lnaof(sin->sin_addr);
	sin = (struct sockaddr_in *)&ifp->if_broadaddr;
	sin->sin_family = AF_INET;
	sin->sin_addr = if_makeaddr(ifp->if_net, INADDR_ANY);
	ifp->if_flags |= IFF_BROADCAST|IFF_NOTRAILERS;
}

/*
 * Start route and arp related stuff
 */
route_arp(ifp, ac)
	struct ifnet *ifp;
	struct arpcom *ac;
{

	if_rtinit(ifp, RTF_UP);
	(void) arpwhohas(ac, &((struct sockaddr_in *)&ifp->if_addr)->sin_addr);
}

/*
 * Prints the name of the interface, i.e. ec0.  Doing this in a
 * subroutine instead of in-line makes several error printf's
 * not have to have the name of the interface hardwire into the
 * printf format string.  Since errors should be infrequent,
 * speed is not an issue.
 */
identify(ifp)
	register struct ifnet *ifp;
{

	printf("%s%d: ",ifp->if_name, ifp->if_unit);
}

/*
 * Checks to see if the packet is a trailer packet.  If so, the
 * offset of the trailer (after the type and remaining length fields)
 * is written into offout and length is changed to reflect the length
 * of the trailer packet (minus the the type and remaining length fields).
 * If it is not a trailer packet, 0 is written into offout and length
 * is left unchanged.
 *
 * The return value is 0 if there is no error, 1 if there is an error.
 */
check_trailer(header, buffer, length, offout)
	struct ether_header *header;
	caddr_t buffer;
	int *length, *offout;
{
	register int off, resid;

	/*
	 * Deal with trailer protocol: if type is PUP trailer
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */
	*offout = 0;
	if (header->ether_type >= ETHERPUP_TRAIL &&
	    header->ether_type < ETHERPUP_TRAIL+ETHERPUP_NTRAILER) {
		off = (header->ether_type - ETHERPUP_TRAIL) << 9;
		if (off >= ETHERMTU)
			return (1);		/* sanity */
		header->ether_type = *(u_short *)((char *)(buffer + off));
		resid = *(u_short *)((char *)(buffer+off+2));
		/*
		 * Can  off + resid  ever be  < length?  If not,
		 * the test should be for  !=  instead of  >, and
		 * *length could be left alone.
		 */
		if (off + resid > *length)
			return (1);		/* sanity */

		/*
		 * Off is now the offset to the start of the
		 * trailer portion, which includes 2 shorts that were
		 * added.  The 2 shorts will be removed by copy_to_mbufs.
		 * The 2 extra shorts contain the real type field and the
		 * length of the trailer.
		 */
		*length = off + resid;
		*offout = off;
	}
	return (0);
}

#ifndef NIT
/*ARGSUSED*/
#endif !NIT
do_protocol(header, m, ap, wirelen)
	struct ether_header *header;
	struct mbuf *m;
	struct arpcom * ap;
	int wirelen;
{
	register struct ifqueue *inq;
#ifdef NIT
	struct nit_ii nii;

	nii.nii_header = (caddr_t)header;
	nii.nii_hdrlen = sizeof (*header);
	nii.nii_type = header->ether_type;
	nii.nii_datalen = wirelen;
	nii.nii_promisc =
	    bcmp((caddr_t)&header->ether_dhost, (caddr_t)&ap->ac_enaddr,
	        sizeof (struct ether_addr)) != 0 &&
	    bcmp((caddr_t)&header->ether_dhost, (caddr_t)&etherbroadcastaddr,
	        sizeof (struct ether_addr)) != 0;
	nit_tap(&ap->ac_if, m, &nii);
	if (nii.nii_promisc) {
		m_freem(m);
		return;
	}
#endif NIT

	/*
	 * Shunt the packet off to the appropriate destination.
	 */
	switch (header->ether_type) {
#ifdef INET
	case ETHERPUP_IPTYPE:
		arpipin(header, m);
		schednetisr(NETISR_IP);
		inq = &ipintrq;
		break;

	case ETHERPUP_ARPTYPE:
		arpinput(ap, m);
		return;

	case ETHERPUP_REVARPTYPE:
		revarpinput(ap, m);
		return;
#endif INET
	default:
		m_freem(m);
		return;
	}

	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		m_freem(m);
		return;
	}
	IF_ENQUEUE(inq, m);
}

/*
 * Routine to copy from an buffer belonging to an interface into mbufs.
 *
 * The buffer consists of a header and a body.  In the normal case,
 * the buffer starts with the header, which is followed immediately
 * by the body.  However, the buffer may contain a trailer packet.
 * In this case, the body appears first and is followed by an expanded
 * header that starts with two additional shorts (containing a type and
 * a header length) and finishes with a header identical in format to
 * that of the normal case.
 *
 * These cases are distinguished by off0, which is the offset to the
 * start of the header part of the buffer.  If nonzero, then we have a
 * trailer packet and must start copying from the beginning of the
 * header as adjusted to skip over its extra leading fields.  Totlen is
 * the overall length of the buffer, including the contribution (if any)
 * of the extra fields in the trailing header.
 */
struct mbuf *
copy_to_mbufs(buffer, totlen, off0)
	caddr_t buffer;
	int totlen, off0;
{
	register caddr_t	cp = buffer;
	caddr_t			cplim = cp + totlen;
	register int		len;
	register struct mbuf	*m;
	struct mbuf		*top = (struct mbuf *) 0,
				**mp = &top;

	if (off0) {
		/*
		 * Trailer: adjust starting postions and lengths
		 * to start of header.
		 */
		cp += off0 + 2 * sizeof (short);
		totlen -= 2 * sizeof (short);
	}

	while (totlen > 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == (struct mbuf *) 0) {
			m_freem(top);
			return ((struct mbuf *) 0);
		}

		/*
		 * If the copy has proceeded past the end of buffer,
		 * move back to the start.  This occurs when the copy
		 * of the header portion of a trailer packet completes.
		 */
		if (cp >= cplim) {
			cp = buffer;
			cplim = cp + off0;
		}

		/*
		 * Get length for the next mbuf of the chain,
		 * allocating a cluster mbuf if the remaining
		 * length justifies it.
		 */
		len = cplim - cp;
		if (len < MCLBYTES || mclget(m) == 0)
			m->m_len = MIN(MLEN, len);
		len = m->m_len;

		bcopy(cp, mtod(m, caddr_t), (u_int) len);
		cp += len;
		totlen -= len;

		*mp = m;
		mp = &m->m_next;
	}
	return (top);
}

/*
 * Routine to copy from mbuf chain to a transmit buffer.
 * Returns the number of bytes copied
 */
copy_from_mbufs(buffer, m)
	register u_char *buffer;
	struct mbuf *m;
{
	register struct mbuf *mp;
	register int off;

	for (off = 0, mp = m; mp; mp = mp->m_next) {
		register u_int len = mp->m_len;
		u_char *mcp;

		if (len != 0) {
			mcp = mtod(mp, u_char *);
			bcopy((caddr_t)mcp, (caddr_t)buffer, len);
			off += len;
			buffer += len;
		}
	}
	m_freem(m);
	return (off);
}

/*
 * Routine to copy from Multibus memory into mbufs.
 *
 * This is how easy it would be if we didn't have to worry about trailers
 */
#ifdef notdef
struct mbuf *
copy_to_mbufs_no_trailers(buffer, totlen)
	u_char *buffer;
	int totlen;
{
	register struct mbuf *m;
	struct mbuf *top = 0, **mp = &top;
	register int len;
	u_char *cp;

	for (cp = buffer; totlen; cp += len, totlen -= len) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0) {
			m_freem(top);
			return (0);
		}
		len = totlen;
		if (len < MCLBYTES || mclget(m) == 0) {
			m->m_len =  MIN(MLEN, len);
			m->m_off = MMINOFF;
		}
		len = m->m_len;
		bcopy((caddr_t)cp, mtod(m,caddr_t), (u_int)len);
		*mp = m;
		mp = &m->m_next;
	}
	return (top);
}
#endif notdef

/*
 * A close relative of m_pullup, differing from it in that:
 *    -	It is not an error for the aggregate length of the subject
 *	mbuf chain to be less than the length requested.
 *    -	Where possible (and convenient), it avoids allocating a
 *	fresh mbuf, instead using the first mbuf of the subject chain.
 *    -	It guarantees that the first mbuf of the resulting chain
 *	has its data portion start at MMINOFF, so that its entire
 *	buffer area is available.
 */
struct mbuf *
ether_pullup(m0, len)
	struct mbuf *m0;
	register int len;
{
	register struct mbuf *m, *n;
	register int count;

	n = m0;
	if (len > MLEN)
		goto bad;
	/*
	 * See whether we can optimize by avoiding mbuf allocation.
	 */
	if (n->m_off == MMINOFF) {
		m = n;
		n = n->m_next;
		if (m->m_len >= len || ! n)
			return (m);
		len -= m->m_len;
	}
	else {
		MGET(m, M_DONTWAIT, n->m_type);
		if (m == 0)
			goto bad;
		m->m_len = 0;
	}
	/*
	 * M names the mbuf we're filling.  N moves through
	 * the mbufs that are drained into m.
	 */
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
	m->m_next = n;
	return (m);
bad:
	m_freem(n);
	return ((struct mbuf *) 0);
}

#ifdef DEBUG
dumpbuffer(header, buffer)
	struct ether_header *header;
	caddr_t buffer;
{
	int i, j;
	short *buf = (short *)buffer;
	int length = buf[1];  /* Specific to IP packets */

	length /= 2;  /* convert to words */
	if (length > 48)
		length = 48;

	if (header->ether_dhost.ether_addr_octet[0] != 0xff) {
		printf("\nSource: ");
		for (i = 0; i < 6; i++)
			printf("%x ",
			    header->ether_shost.ether_addr_octet[i]&0xff);
		printf("\nDestin: ");
		for (i = 0; i < 6; i++)
			printf("%x ",
			    header->ether_dhost.ether_addr_octet[i]&0xff);
		printf("\nBuffer");
		j = 0;
		for (i = 0; i < length; i++) {
			if (j == 0)
				printf("\n");
			if (++j == 16)
				j = 0;
			printf("%x ", buf[i]&0xffff);
		}
		printf("\n");
	} else
		printf(" Broadcast ");
}

chkmbuf()
{
}
#endif DEBUG

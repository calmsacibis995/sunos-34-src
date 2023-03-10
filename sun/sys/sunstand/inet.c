#ifndef lint
static	char sccsid[] = "@(#)inet.c	1.1 86/09/25	Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Standalone IP send and receive - specific to Ethernet
 * Includes ARP and Reverse ARP
 */
#include "saio.h"
#include "../h/socket.h"
#include "../net/if.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "sainet.h"
#include "../mon/sunromvec.h"
#include "../mon/idprom.h"

#define millitime() (*romp->v_nmiclock)

struct ether_addr etherbroadcastaddr = { 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

struct arp_packet {
	struct ether_header	arp_eh;
	struct ether_arp	arp_ea;
#define	used_size (sizeof (struct ether_header)+sizeof(struct ether_arp))
	char	filler[ETHERMIN - sizeof(struct ether_arp)];
};

#define WAITCNT	2	/* 4 seconds before bitching about arp/revarp */

/*
 * Fetch our Ethernet address from the ID prom
 */
myetheraddr(ea)
	struct ether_addr *ea;
{
	struct idprom id;

	if (idprom(IDFORM_1, &id) != IDFORM_1) {
		printf("ERROR: missing or invalid ID prom\n");
		return;
	}
	*ea = *(struct ether_addr *)id.id_ether;
}

/*
 * Initialize IP state
 * Find out our Ethernet address and call Reverse ARP
 * to find out our Internet address
 * Set the ARP cache to the broadcast host
 */
inet_init(sip, sain, tmpbuf)
	register struct saioreq *sip;
	register struct sainet *sain;
	char *tmpbuf;
{
	myetheraddr(&sain->sain_myether);
	sain->sain_hisaddr.s_addr = 0;
	sain->sain_hisether = etherbroadcastaddr;
	revarp(sip, sain, tmpbuf);
}


/*
 * Output an IP packet
 * Cause ARP to be invoked if necessary
 */
ip_output(sip, buf, len, sain, tmpbuf)
	register struct saioreq *sip;
	caddr_t buf, tmpbuf;
	short len;
	register struct sainet *sain;
{
	register struct ether_header *eh;
	register struct ip *ip;

	eh = (struct ether_header *)buf;
	ip = (struct ip *)(buf + sizeof(struct ether_header));
	if (ip->ip_dst.s_addr != sain->sain_hisaddr.s_addr) {
		sain->sain_hisaddr.s_addr = ip->ip_dst.s_addr;
		arp(sip, sain, tmpbuf);
	}
	eh->ether_type = ETHERTYPE_IP;
	eh->ether_shost = sain->sain_myether;
	eh->ether_dhost = sain->sain_hisether;
	/* checksum the packet */
	ip->ip_sum = 0;
	ip->ip_sum = ipcksum((caddr_t)ip, sizeof (struct ip));
	if (len < ETHERMIN+sizeof(struct ether_header))
		len = ETHERMIN+sizeof(struct ether_header);
	return (*sip->si_sif->sif_xmit)(sip->si_devdata, buf, len);
}

/*
 * Check incoming packets for IP packets
 * addressed to us. Also, respond to ARP packets
 * that wish to know about us.
 * Returns a length for any IP packet addressed to us, 0 otherwise.
 */
ip_input(sip, buf, sain)
	register struct saioreq *sip;
	caddr_t buf;
	register struct sainet *sain;
{
	register short len;
	register struct ether_header *eh;
	register struct ip *ip;
	register struct ether_arp *ea;

	len = (*sip->si_sif->sif_poll)(sip->si_devdata, buf);
	eh = (struct ether_header *)buf;
	if (eh->ether_type == ETHERTYPE_IP &&
	    len >= sizeof(struct ether_header)+sizeof(struct ip)) {
		ip = (struct ip *)(buf + sizeof(struct ether_header));
#ifdef NOREVARP
		if ((sain->sain_hisaddr.s_addr & 0xFF000000) == 0 &&
		    bcmp((caddr_t)&etherbroadcastaddr,
				(caddr_t)&eh->ether_dhost, 6) != 0 &&
		    (in_broadaddr(sain->sain_hisaddr) ||
		    in_lnaof(ip->ip_src) == in_lnaof(sain->sain_hisaddr))) {
			sain->sain_myaddr = ip->ip_dst;
			sain->sain_hisaddr = ip->ip_src;
			sain->sain_hisether = eh->ether_shost;
		}
#endif
		if (ip->ip_dst.s_addr != sain->sain_myaddr.s_addr) 
			return (0);
		return (len);
	}
	if (eh->ether_type == ETHERTYPE_ARP &&
	    len >= sizeof(struct ether_header)+sizeof(struct ether_arp)) {
		ea = (struct ether_arp *)(buf + sizeof(struct ether_header));
		if (ea->arp_pro != ETHERTYPE_IP)
			return (0);
		if (arp_spa(ea).s_addr == sain->sain_hisaddr.s_addr)
			sain->sain_hisether = arp_sha(ea);
		if (ea->arp_op == ARPOP_REQUEST &&
		    arp_tpa(ea).s_addr == sain->sain_myaddr.s_addr) {
			ea->arp_op = ARPOP_REPLY;
			eh->ether_dhost = arp_sha(ea);
			eh->ether_shost = sain->sain_myether;
			arp_tha(ea) = arp_sha(ea);
			arp_tpa(ea) = arp_spa(ea);
			arp_sha(ea) = sain->sain_myether;
			arp_spa(ea) = sain->sain_myaddr;
			(*sip->si_sif->sif_xmit)(sip->si_devdata, buf, 
						sizeof(struct arp_packet));
		}
		return (0);
	}
	return (0);
}

/*
 * arp
 * Broadcasts to determine Ethernet address given IP address
 * See RFC 826
 */
arp(sip, sain, tmpbuf)
	register struct saioreq *sip;
	register struct sainet *sain;
	char *tmpbuf;
{
	struct arp_packet out;

	if (in_broadaddr(sain->sain_hisaddr)
#ifdef NOREVARP
	    || (sain->sain_hisaddr.s_addr & 0xFF000000) == 0
#endif
	    ) {
		sain->sain_hisether = etherbroadcastaddr;
		return;
	}
	out.arp_eh.ether_type = ETHERTYPE_ARP;
	out.arp_ea.arp_op = ARPOP_REQUEST;
	arp_tha(&out.arp_ea) = etherbroadcastaddr;	/* what we want */
	arp_tpa(&out.arp_ea).s_addr = sain->sain_hisaddr.s_addr;
	comarp(sip, sain, &out, tmpbuf);
}

/*
 * Reverse ARP client side
 * Determine our Internet address given our Ethernet address
 * See RFC 903
 */
revarp(sip, sain, tmpbuf)
	register struct saioreq *sip;
	register struct sainet *sain;
	char *tmpbuf;
{
	struct arp_packet out;

#ifdef NOREVARP
	sain->sain_myaddr.s_addr = 0;
	bcopy((caddr_t)&sain->sain_myether.ether_addr_octet[3],
		(caddr_t)(&sain->sain_myaddr)+1, 3);
#else
	out.arp_eh.ether_type = ETHERTYPE_REVARP;
	out.arp_ea.arp_op = REVARP_REQUEST;
	arp_tha(&out.arp_ea) = sain->sain_myether;
	arp_tpa(&out.arp_ea).s_addr = 0;/* what we want to find out */
	comarp(sip, sain, &out, tmpbuf);
#endif
}

/*
 * Common ARP code 
 * Broadcast the packet and wait for the right response.
 * Fills in *sain with the results
 */
comarp(sip, sain, out, tmpbuf)
	register struct saioreq *sip;
	register struct sainet *sain;
	register struct arp_packet *out;
	char *tmpbuf;
{
	register struct arp_packet *in = (struct arp_packet *)tmpbuf;
	register int e, count, time, feedback,len, delay = 2;
	char    *ind = "-\|/";

	out->arp_eh.ether_dhost = etherbroadcastaddr;
	out->arp_eh.ether_shost = sain->sain_myether;
	out->arp_ea.arp_hrd =  ARPHRD_ETHER;
	out->arp_ea.arp_pro = ETHERTYPE_IP;
	out->arp_ea.arp_hln = sizeof(struct ether_addr);
	out->arp_ea.arp_pln = sizeof(struct in_addr);
	arp_sha(&out->arp_ea) = sain->sain_myether;
	arp_spa(&out->arp_ea).s_addr = sain->sain_myaddr.s_addr;
	feedback = 0;

	for (count=0; ; count++) {
		if (count == WAITCNT) {
			if (out->arp_ea.arp_op == ARPOP_REQUEST) {
				printf("\nRequesting Ethernet address for ");
				inet_print(arp_tpa(&out->arp_ea));
			} else {
				printf("\nRequesting Internet address for ");
				ether_print(&arp_tha(&out->arp_ea));
			}
		}
		e = (*sip->si_sif->sif_xmit)(sip->si_devdata, (caddr_t)out,
			sizeof *out);
		if (e)
			printf("X\b");
		else
			printf("%c\b", ind[feedback++ % 4]); /* Show activity */

		time = millitime() + (delay * 1000);	/* broadcast delay */
		while (millitime() <= time) {
			len = (*sip->si_sif->sif_poll)(sip->si_devdata, tmpbuf);
			if (len < used_size)
				continue;
			if (in->arp_ea.arp_pro != ETHERTYPE_IP)
				continue;
			if (out->arp_ea.arp_op == ARPOP_REQUEST) {
				if (in->arp_eh.ether_type != ETHERTYPE_ARP)
					continue;
				if (in->arp_ea.arp_op != ARPOP_REPLY)
					continue;
				if (arp_spa(&in->arp_ea).s_addr !=
				    arp_tpa(&out->arp_ea).s_addr)
					continue;
				if (count >= WAITCNT) {
					printf("Found at ");
					ether_print(&arp_sha(&in->arp_ea));
				}
				sain->sain_hisether = arp_sha(&in->arp_ea);
				return;
			} else {		/* Reverse ARP */
				if (in->arp_eh.ether_type !=ETHERTYPE_REVARP)
					continue;
				if (in->arp_ea.arp_op != REVARP_REPLY)
					continue;
				if (bcmp((caddr_t)&arp_tha(&in->arp_ea),
				    (caddr_t)&arp_tha(&out->arp_ea), 
				    sizeof (struct ether_addr)) != 0)
					continue;

				if (count >= WAITCNT) {
					printf("Internet address is ");
					inet_print(arp_tpa(&in->arp_ea));
				}
				sain->sain_myaddr = arp_tpa(&in->arp_ea);
				/* short circuit first ARP */
				sain->sain_hisaddr = arp_spa(&in->arp_ea);
				sain->sain_hisether = arp_sha(&in->arp_ea);
				return;
			}
		}

		delay = delay * 2;	/* Double the request delay */
		if (delay > 64)		/* maximum delay is 64 seconds */
			delay = 64;

		(*sip->si_sif->sif_reset)(sip->si_devdata);
	}
	/* NOTREACHED */
}

/*
 * Return the host portion of an internet address.
 */
in_lnaof(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);

	if (IN_CLASSA(i))
		return ((i)&IN_CLASSA_HOST);
	else if (IN_CLASSB(i))
		return ((i)&IN_CLASSB_HOST);
	else
		return ((i)&IN_CLASSC_HOST);
}

/*
 * Test for broadcast IP address
 */
in_broadaddr(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);

	if (IN_CLASSA(i)) {
		i &= IN_CLASSA_HOST;
		return (i == 0 || i == 0xFFFFFF);
	} else if (IN_CLASSB(i)) {
		i &= IN_CLASSB_HOST;
		return (i == 0 || i == 0xFFFF);
	} else if (IN_CLASSC(i)) {
		i &= IN_CLASSC_HOST;
		return (i == 0 || i == 0xFF);
	}
	/* NOTREACHED */
}

/*
 * Compute one's complement checksum
 * for IP packet headers 
 */
ipcksum(cp, count)
	caddr_t	cp;
	register unsigned short	count;
{
	register unsigned short	*sp = (unsigned short *)cp;
	register unsigned long	sum = 0;
	register unsigned long	oneword = 0x00010000;

	count >>= 1;
	while (count--) {
		sum += *sp++;
		if (sum >= oneword) {		/* Wrap carries into low bit */
			sum -= oneword;
			sum++;
		}
	}
	return (~sum);
}

inet_print(s)
	struct in_addr s;
{
	printf("%d.%d.%d.%d\n",
		s.S_un.S_un_b.s_b1, s.S_un.S_un_b.s_b2, 
		s.S_un.S_un_b.s_b3, s.S_un.S_un_b.s_b4);
}

ether_print(ea)
	struct ether_addr *ea;
{
	register u_char *p = &ea->ether_addr_octet[0];

	printf("%x:%x:%x:%x:%x:%x\n", p[0], p[1], p[2], p[3], p[4], p[5]);
}

#ifndef lint
static	char sccsid[] = "@(#)nd.c	1.1 86/09/25	Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * standalone nd.c
 * Used only in /boot after TFTP boot.
 * Never in PROM or boot blocks
 */

/* This value is used in header files, it must be at least 1 */
#define	NND	1

#include "saio.h"
#include "param.h"
#include "../h/socket.h"
#include "../net/if.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "sainet.h"
#include "../sun/ndio.h"
#include "../mon/sunromvec.h"
#include "../mon/idprom.h"

#define millitime() (*romp->v_nmiclock)

/* 
 * Each network device driver must start off their local variables
 * structure (pointed to by sip->si_devdata) by a struct nd or a union
 * of a struct nd and other protocol structures.  This reserves the
 * space needed by the protocol handler(s) without any further ado.
 * Therefore, the protocol can simply assume that sip->si_devdata
 * points to its parameters.
 */
#define	NDBASE	(struct nd *)(sip->si_devdata)

/* standalone RAM variables */
struct nd {			
	int	nd_seq;			/* current sequence number */
	struct ether_header nd_xh;	/* xmit header and packet */
	struct ndpack nd_xp;		/* must be contiguous */
	int	nd_block;		/* block number of data in "cache" */
	char	nd_data[NDMAXDATA];	/* "cache" of receive data */
	char	nd_buf[1600];		/* temp buf for packets */
	struct	sainet nd_inet;		/* INET state */
};

#define	NDHDRSIZE	(sizeof(struct ether_header) + sizeof(struct ndpack))

/*
 * Zap this to indicate ND device from which to boot by default
 */
short ndbootdev = 0x40;		/* public partition #0 */

/*
 * Initialize network disk operation.
 * Get server address and partition number from si_unit and si_boff.
 */
etheropen(sip)
	struct saioreq *sip;
{
	register struct nd *nd = NDBASE;
	register i;
	char buf[DEV_BSIZE];

	bzero((caddr_t)nd, sizeof *nd);

	/*
	 * Initialize INET state
	 */
	inet_init(sip, &nd->nd_inet, nd->nd_buf);

	/* 
	 * Set src and dst host addresses
	 * Dst host is argument with our net number plugged in
	 */
	nd->nd_xp.np_ip.ip_src = nd->nd_inet.sain_myaddr;
	nd->nd_xp.np_ip.ip_dst.s_addr = sip->si_unit;
	nd->nd_xp.np_ip.ip_dst.s_addr += nd->nd_xp.np_ip.ip_src.s_addr -
					in_lnaof(nd->nd_xp.np_ip.ip_src);
/* fix for FF broadcast ? */
	 
	nd->nd_block = -10;		/* impossible block number */
	nd->nd_seq = millitime();	/* initial sequence number */
	/* set ether, ip and ndpack fixed fields */
	nd->nd_xp.np_ip.ip_v = IPVERSION;
	nd->nd_xp.np_ip.ip_hl = sizeof (struct ip) / 4;
	nd->nd_xp.np_ip.ip_len = sizeof (struct ndpack);
	nd->nd_xp.np_ip.ip_ttl = NDXTIMER;
	nd->nd_xp.np_ip.ip_p = IPPROTO_ND;
	nd->nd_xp.np_op = (NDOPREAD | NDOPWAIT);
	nd->nd_xp.np_min = (int)sip->si_boff;
	/*
	 * We XOR here to keep similar semantics of bie(,,40)
	 * to mean private when default is public
	 */
	nd->nd_xp.np_min ^= ndbootdev;
	nd->nd_xp.np_bcount = NDMAXDATA;

	/* try to read block 0 */
	if (ndread(sip, 0, buf))
		return (-1);
	return (0);
}

/*
 * Strategy routine.  User interface to do reads and writes is thru here.
 * We currently don't support writes.
 */
int
etherstrategy(sip, rw)
	register struct saioreq *sip;
	int rw;
{
	int cnt = sip->si_cc;
	int blk = sip->si_bn;
	char *ma = sip->si_ma;

	if (rw == WRITE)
		return (0);
	while (cnt > 0) {
		if (ndread(sip, blk, ma))
			break;
		blk++;
		ma += DEV_BSIZE;
		cnt -= DEV_BSIZE;
	}
	return (sip->si_cc - cnt);
}

/*
 * Receive packets from an nd server.
 * We keep a 1K cache to speed things up
 */
ndread(sip, block, caddr)
	struct saioreq *sip;
	int block;
	caddr_t caddr;
{
	register struct nd *nd = NDBASE;
	register char *cp;
	register short ok, i;

	cp = nd->nd_data;
	if (block == nd->nd_block)
		goto copy;
	if (block == (nd->nd_block+1)) {
		cp += DEV_BSIZE;
		goto copy;
	}
	/*
	 * Fill in the request block
	 */
	nd->nd_xp.np_blkno = block;
	nd->nd_xp.np_seq = ++nd->nd_seq;

#define	LIMIT	500
	for (i=0 ; i<LIMIT ; i++) {
		if (ip_output(sip, (caddr_t)&nd->nd_xh, NDHDRSIZE,
		    &nd->nd_inet, nd->nd_buf)) {
			printf("X\010");
			continue;
		}
		ok = ndrecv(sip);
		if (ok >= 0)
			break;
		printf("?\b");	/* Indicate there's a problem */
	}
	if (i == LIMIT) {
		printf("nd: no file server, giving up.\n");
		return (-1);
	}
	if (ok == 0)
		return (-1);
	nd->nd_block = block;
copy:
	bcopy(cp, caddr, DEV_BSIZE);
	return (0);
}

#define	NDREPLYTIME	(2*1000)	/* miilliseconds between retransmits */

ndrecv(sip)
	struct saioreq *sip;
{
	register struct nd *nd = NDBASE;
	register struct ndpack *np;
	register struct ether_header *eh;
	register int time;
	register short len;

	time = millitime() + NDREPLYTIME;
	for (;;) {
		if (millitime() > time) {
			(*sip->si_sif->sif_reset)(sip->si_devdata, sip);
			return (-1);
		}
		len = ip_input(sip, nd->nd_buf, &nd->nd_inet);
		if (len < NDHDRSIZE) 
			continue;
		eh = (struct ether_header *)nd->nd_buf;
		np = (struct ndpack *)&eh[1];

		/*
		 * avoid bug in older servers.
		 * The problem is that pre-Sun Unix 1.1 servers would
		 * respond to ANY directed boot request that they
		 * received, whether or not it was directed to their
		 * Internet address.  (They assumed that the boot
		 * proms knew their Ethernet address.)  We now broadcast
		 * all boot requests, directed or otherwise, so we have
		 * to filter out replies from ones we don't care about.
		 */
		if (in_lnaof(nd->nd_xp.np_ip.ip_dst) != 0 &&
		    in_lnaof(nd->nd_xp.np_ip.ip_dst) !=
		    in_lnaof(np->np_ip.ip_src))
			continue;

		if (np->np_ip.ip_p == IPPROTO_ND && np->np_seq == nd->nd_seq)
			break;
	}

	if (np->np_op == NDOPERROR) {
		printf("nd: err %x bn %d\n", np->np_error, np->np_blkno);
		return (0);
	}
	if (np->np_op != (NDOPREAD|NDOPDONE|NDOPWAIT) ||
	    np->np_ccount != NDMAXDATA ||
	    np->np_ip.ip_len != (sizeof(struct ndpack)+NDMAXDATA)) {
#ifdef DEBUG
		printf("ND protocol violation\n");
#endif DEBUG
		return (-1);
	}
	/* lock onto server if we are still broadcasting */
	if (in_broadaddr(nd->nd_xp.np_ip.ip_dst))
		nd->nd_xp.np_ip.ip_dst = np->np_ip.ip_src;
#ifdef NOREVARP
	if ((nd->nd_xp.np_ip.ip_dst.s_addr & 0xFF000000) == 0)
		nd->nd_xp.np_ip.ip_dst = np->np_ip.ip_src;
#endif
	if ((nd->nd_seq & 7) == 0)
		printf((nd->nd_seq & 8) ? "-\b" : "=\b");
	bcopy(nd->nd_buf + NDHDRSIZE, nd->nd_data, NDMAXDATA);
	return (1);
}

#ifndef NDBOOT
#ifndef lint
static	char sccsid[] = "@(#)nd.c	1.1 84/12/21	Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * nd.c
 * Standalone net disk protocol module.
 * Used in PROMs, /boot and bootnd [blocks 1-15].
 */

/* This value is used in header files, it must be at least 1 */
#define	NND	1

/*
 * #define NDBOOT to get PROM version.  This is done by PROM sunconfig.
 */

#include "saio.h"
#include "../h/socket.h"
#include "../net/if.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../sun/ndio.h"
#include "../mon/sunromvec.h"
#include "../mon/s2addrs.h"
#include "../mon/idprom.h"

#define millitime() (*romp->v_nmiclock)

#define	NDBASE	(struct nd *)0x3000		/* standalone RAM at 12->16K */
#define	DATASIZE NDMAXDATA			/* size of each read */

struct nd {			/* standalone RAM variables */
	int	nd_seq;		/* current sequence number */
	struct ether_header nd_xh;	/* xmit header and packet */
	struct ndpack nd_xp;
	int	nd_block;	/* starting block number of data in "cache" */
	char	nd_data[DATASIZE]; /* "cache" of receive data */
	char	nd_buf[1600];	/* temp buf for packets */
	short	nd_efound;	/* found my ether addr */
	struct ether_addr nd_eaddr;	/* my ether addr */
};

struct ether_addr etherbroadcastaddr = { 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

/*
 * Strategy routine.  User interface to do reads and writes is thru here.
 * We currently don't support writes.
 */
int
ndstrategy(sip,rw)
	register struct saioreq *sip;
	int rw;
{
	int cnt = sip->si_cc;
	int blk = sip->si_bn;
	char *ma = sip->si_ma;

	if (rw == WRITE)
		goto out;
	while (cnt > 0) {
		if (ndread(sip, blk, ma))
			break;
		blk++;
		ma += DEV_BSIZE;
		cnt -= DEV_BSIZE;
	}
out:
	return (sip->si_cc - cnt);
}

ndinit()
{
	register struct nd *nd = NDBASE;
	struct idprom id;

	bzero((caddr_t)nd, sizeof *nd);

	if (IDFORM_1 != idprom(IDFORM_1, &id)) return;
	localetheraddr((struct ether_addr *)(id.id_ether),
		(struct ether_addr *)NULL);
}

/*
 * Initialize network disk operation.
 * Get server address and partition number from si_unit and si_boff.
 */
ndopen(sip)
	struct saioreq *sip;
{
	register struct nd *nd = NDBASE;
	register i;
	char buf[DEV_BSIZE];

	nd->nd_block = -10;		/* impossible block number */
	nd->nd_seq = millitime();	/* initial sequence number */

	/* 
	 * Set src and dst host addresses;  ARP is probably too big
	 * for PROM/bootxx;  boot protocol should be separate from nd.
	 */
	nd->nd_xh.ether_dhost = etherbroadcastaddr; /* broadcast */
	nd->nd_xp.np_ip.ip_dst.s_addr = sip->si_unit;
	localetheraddr(NULL, &nd->nd_xh.ether_shost);
	bcopy((caddr_t)&nd->nd_xh.ether_shost.ether_addr_octet[3],
		(caddr_t)(&nd->nd_xp.np_ip.ip_src)+1, 3);
	 
	/* set ether, ip and ndpack fixed fields */
	nd->nd_xh.ether_type = ETHERPUP_IPTYPE;
	nd->nd_xp.np_ip.ip_v = IPVERSION;
	nd->nd_xp.np_ip.ip_hl = sizeof (struct ip) / 4;
	nd->nd_xp.np_ip.ip_len = sizeof (struct ndpack);
	nd->nd_xp.np_ip.ip_ttl = NDXTIMER;
	nd->nd_xp.np_ip.ip_p = IPPROTO_ND;
	nd->nd_xp.np_op = (NDOPREAD | NDOPWAIT);
	nd->nd_xp.np_min = (NDMINPUBLIC ^ (int)sip->si_boff);
	nd->nd_xp.np_bcount = (DATASIZE);

	/* try to read block 0 */
	if (ndread(sip, 0, buf)) {
		return (-1);
	}
	return (0);
}

ndxmit(sip)
	struct saioreq *sip;
{
	register struct nd *nd = NDBASE;
	int e;

#ifdef DEBUG
printf("nd xmit %x\n", nd->nd_xp.np_seq);
#endif DEBUG
	e = (*sip->si_sif->sif_xmit) (sip->si_devdata, (caddr_t)&nd->nd_xh,
		sizeof nd->nd_xh + sizeof nd->nd_xp);
	if (e)
		printf("X\010");
}

ndrecv(sip)
	struct saioreq *sip;
{
	register struct nd *nd = NDBASE;
	register struct ndpack *np;
	register struct ether_header *eh;
	int time;
	short len, *tp;
	char *dp;

	time = millitime() + (2 * 1000);	/* 2 seconds */
	for (;;) {
		if (millitime() > time) {
			(*sip->si_sif->sif_reset)(sip->si_devdata);
			return (-1);
		}
		len = (*sip->si_sif->sif_poll)(sip->si_devdata, nd->nd_buf);
		if (len < (sizeof(struct ether_header)+ sizeof(struct ndpack))) {
#ifdef DEBUG
			if (len > 0) printf("Skip %d byte packet\n", len);
#endif DEBUG
			continue;
		}

		eh = (struct ether_header *)nd->nd_buf;
		if (bcmp((caddr_t)&eh->ether_dhost,
		    (caddr_t)&etherbroadcastaddr, 6) == 0) {
#ifdef DEBUG
			printf("nd: received broadcast packet\n");
#endif DEBUG
			continue;
		}
		if (eh->ether_type == ETHERPUP_TRAIL + 2) {
			dp = (char *)&eh[1];
			tp = (short *)(dp+1024);
			eh->ether_type = *tp;
			np = (struct ndpack *)&tp[2];
		} else {
			np = (struct ndpack *)&eh[1];
			dp = (char *)&np[1];
		}

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
		if (nd->nd_xp.np_ip.ip_dst.s_addr != 0 &&
		    in_lnaof(nd->nd_xp.np_ip.ip_dst) !=
		    in_lnaof(np->np_ip.ip_src)) {
#ifdef DEBUG
			printf("Skip reply from ip %x\n",
				in_lnaof(np->np_ip.ip_src));
#endif DEBUG
			continue;
		}

		if (eh->ether_type == ETHERPUP_IPTYPE &&
		    np->np_ip.ip_p == IPPROTO_ND &&
		    np->np_seq == nd->nd_seq)
			break;
#ifdef DEBUG
		printf("Skip bad packet type %x proto %x seq %x\n", 
			eh->ether_type, np->np_ip.ip_p, np->np_seq);
#endif DEBUG
	}

#ifdef DEBUG
	printf("nd recv %x\n", np->np_seq);
#endif DEBUG

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
	if (!bcmp((caddr_t)&nd->nd_xh.ether_dhost,
	    (caddr_t)&etherbroadcastaddr, 6)) {
		nd->nd_xh.ether_dhost = eh->ether_shost;
		nd->nd_xp.np_ip.ip_dst = np->np_ip.ip_src;
		nd->nd_xp.np_ip.ip_src = np->np_ip.ip_dst;
	}
	if ((nd->nd_seq & 7) == 0)
		printf((nd->nd_seq & 8) ? "-\b" : "=\b");
	bcopy(dp, nd->nd_data, DATASIZE);
	return (1);
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

ndread(sip, block, caddr)
	struct saioreq *sip;
	int block;
	caddr_t caddr;
{
	register struct nd *nd = NDBASE;
	char *cp;
	int ok, i, limit = 500;

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
	nd->nd_xp.np_ip.ip_sum = 0;
	nd->nd_xp.np_ip.ip_sum =
		ndcksum((caddr_t)&nd->nd_xp, sizeof (struct ip));

	for (i=0 ; i<limit ; i++) {
		ndxmit(sip);
		ok = ndrecv(sip);
		if (ok >= 0)
			break;
		printf("?\b");	/* Indicate there's a problem */
	}
	if (i == limit) {
		printf("nd: no file server, giving up.\n");
		return (-1);
	}
	if (!ok)
		return (-1);
	nd->nd_block = block;
copy:
	bcopy(cp, caddr, DEV_BSIZE);
	return (0);
}

ndcksum(cp, count)
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


/*
 * Set this machine's Ethernet address.  The first caller gets it.
 * Subsequent calls are ignored.
 */
localetheraddr(hint, result)
	struct ether_addr *hint, *result;
{
	register struct nd *nd = NDBASE;

	if (!nd->nd_efound) {
		nd->nd_efound = 1;
		if (hint == NULL)
			return (0);
		nd->nd_eaddr = *hint;
	}
	if (result != NULL)
		*result = nd->nd_eaddr;
	return (1);
}

#ifndef lint
static  char sccsid[] = "@(#)rpc.etherd.c 1.1 86/09/25 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include <ctype.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <netinet/udp.h>
#include <net/route.h>
#include <net/nit.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/udp_var.h>
#include <netinet/ip_icmp.h>
#include <rpcsvc/ether.h>

struct  sample {
        struct  ether_header ts_eheader;
        union {
                struct  udpiphdr tsu_udpipheader;
                struct ether_arp tsu_arpheader;
        } tsu;
#define ts_ipovlyheader tsu.tsu_udpipheader.ui_i
#define ts_udpheader    tsu.tsu_udpipheader.ui_u
#define ts_arpheader    tsu.tsu_arpheader
};

#define BUFSPACE        4*8192
#define CHUNKSIZE       2048
#define SNAPLEN         (sizeof (struct sample))

int bufspace = BUFSPACE;
int chunksize = CHUNKSIZE;
int snaplen = SNAPLEN;
char	buf[BUFSPACE];
struct etherhmem *srchtable[HASHSIZE];
struct etherhmem *dsthtable[HASHSIZE];

extern int errno;
int senddata();
int if_fd = -1;
char *device;
int net;
struct etherstat etherstat;
struct etheraddrs etheraddrs;
struct addrmask srcmask, dstmask, protomask, lnthmask;

main(argc, argv)
	char **argv;
{
	SVCXPRT *transp;
	int readfds, s, ok;
	struct timeval timeout;
	int readbit, rpcbit;
	struct sockaddr_in addr;
	
	if (argc < 2) {
		fprintf(stderr, "Usage: etherd interface\n");
		exit(1);
	}
	device = argv[1];
	initdevice();
	s = socket(AF_INET, SOCK_DGRAM, 0);
	transp = svcudp_create(s);
	if (transp == NULL) {
		fprintf(stderr, "couldn't create an RPC server\n");
		exit(1);
	}
	pmap_unset(ETHERSTATPROG, ETHERSTATVERS);
	if (!svc_register(transp, ETHERSTATPROG, ETHERSTATVERS,
	    senddata, IPPROTO_UDP)) {
		fprintf(stderr, "couldn't register ETHERSTAT service\n");
		exit(1);
	}
	/* 
	 * what if this machine is a gateway?
	 */
	get_myaddress(&addr);
	net = inet_netof(addr);
	
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;
	readbit = 1 << if_fd;
	rpcbit = 1 << s;
	/* 
	 * should default be to init device?
	 */
	deinitdevice();
	for (;;) {
		if (if_fd < 0)
			readfds = rpcbit;
		else
			readfds = rpcbit | readbit;
		if (select(32, &readfds, 0, 0, 0) < 0) {
			if (errno == EINTR)
				continue;
			perror("etherd: select");
			exit(1);
		}
		if (readfds & readbit)
			readdata();
		if (readfds & rpcbit)
			svc_getreq(1 << s);
	}
}

senddata(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	switch (rqstp->rq_proc) {
		case NULLPROC:
			if (!svc_sendreply(transp, xdr_void, 0)) {
				fprintf(stderr,"couldn't reply to RPC call\n");
				exit(1);
			}
			return;
		case ETHERSTATPROC_GETDATA:
			/* if (!svc_getargs(transp, xdr_void, NULL)) {
				svcerr_decode(transp);
				return;
			} */
			gettimeofday(&etherstat.e_time, 0);
			if (!svc_sendreply(transp, xdr_etherstat, &etherstat)) {
				fprintf(stderr,"couldn't reply to RPC call\n");
				exit(1);
			}
			return;
		case ETHERSTATPROC_GETSRCDATA:
			/* if (!svc_getargs(transp, xdr_void, NULL)) {
				svcerr_decode(transp);
				return;
			} */
			gettimeofday(&etheraddrs.e_time, 0);
			etheraddrs.e_addrs = srchtable;
			if (!svc_sendreply(transp, xdr_etheraddrs,
			    &etheraddrs)) {
				fprintf(stderr,"couldn't reply to RPC call\n");
				exit(1);
			}
			return;
		case ETHERSTATPROC_GETDSTDATA:
			/* if (!svc_getargs(transp, xdr_void, NULL)) {
				svcerr_decode(transp);
				return;
			} */
			gettimeofday(&etheraddrs.e_time, 0);
			etheraddrs.e_addrs = dsthtable;
			if (!svc_sendreply(transp, xdr_etheraddrs,
			    &etheraddrs)) {
				fprintf(stderr,"couldn't reply to RPC call\n");
				exit(1);
			}
			return;
		case ETHERSTATPROC_ON:
			/* if (!svc_getargs(transp, xdr_void, NULL)) {
				svcerr_decode(transp);
				return;
			} */
			initdevice();
			if (!svc_sendreply(transp, xdr_void, NULL)) {
				fprintf(stderr,"couldn't reply to RPC call\n");
				exit(1);
			}
			return;
		case ETHERSTATPROC_OFF:
			/* if (!svc_getargs(transp, xdr_void, NULL)) {
				svcerr_decode(transp);
				return;
			} */
			deinitdevice();
			if (!svc_sendreply(transp, xdr_void, NULL)) {
				fprintf(stderr,"couldn't reply to RPC call\n");
				exit(1);
			}
			return;
		case ETHERSTATPROC_SELECTSRC:
			if (!svc_getargs(transp, xdr_addrmask, &srcmask)) {
				svcerr_decode(transp);
				return;
			}
			if (!svc_sendreply(transp, xdr_void, NULL)) {
				fprintf(stderr,"couldn't reply to RPC call\n");
				exit(1);
			}
			return;
		case ETHERSTATPROC_SELECTDST:
			if (!svc_getargs(transp, xdr_addrmask, &dstmask)) {
				svcerr_decode(transp);
				return;
			}
			if (!svc_sendreply(transp, xdr_void, NULL)) {
				fprintf(stderr,"couldn't reply to RPC call\n");
				exit(1);
			}
			return;
		case ETHERSTATPROC_SELECTPROTO:
			if (!svc_getargs(transp, xdr_addrmask, &protomask)) {
				svcerr_decode(transp);
				return;
			}
			if (!svc_sendreply(transp, xdr_void, NULL)) {
				fprintf(stderr,"couldn't reply to RPC call\n");
				exit(1);
			}
			return;
		case ETHERSTATPROC_SELECTLNTH:
			if (!svc_getargs(transp, xdr_addrmask, &lnthmask)) {
				svcerr_decode(transp);
				return;
			}
			if (!svc_sendreply(transp, xdr_void, NULL)) {
				fprintf(stderr,"couldn't reply to RPC call\n");
				exit(1);
			}
			return;
		default:
			svcerr_noproc(transp);
			return;
	}
}

readdata()
{
	register struct sample *sp;
	register struct etherstat *p;
	register struct etheraddrs *q;
	register struct ip *ip;
	register type;
	caddr_t bp, bufstop;
	struct nit_hdr *nh;
	int datalen;
	char *edst, *esrc;
	int cc;

	p = &etherstat;
	q = &etheraddrs;
drop_packet:
	if ((cc = read(if_fd, buf, sizeof (buf))) < 0) {
		perror("etherd: read");
		exit(1);
	}
	/*
	 * Loop through each packet.  The increment expression
	 * rounds up to the next int boundary past the end of
	 * the previous packet.
	 */
	bufstop = buf + cc;
	for (bp = buf; bp < bufstop;
	    bp += ((sizeof(struct nit_hdr)+datalen+sizeof(int)-1)
			& ~(sizeof (int)-1))) {
		nh = (struct nit_hdr *)bp;
		sp = (struct sample *)(bp + sizeof(*nh));

		switch (nh->nh_state) {
		case NIT_CATCH:
			datalen = nh->nh_datalen;
			break;
		case NIT_SEQNO:
		case NIT_NOMBUF:
		case NIT_NOCLUSTER:
		case NIT_NOSPACE:
			datalen = 0;
			continue;
		default:
			fprintf(stderr, "bad nit state %d\n", nh->nh_state);
			goto drop_packet;
		}

		if (nh->nh_wirelen < MINPACKETLEN) {
			edst = (char *)&sp->ts_eheader.ether_dhost;
			esrc = (char *)&sp->ts_eheader.ether_shost;
			fprintf(stderr,
			    "bad lnth %d dst %08x%04x src %08x%04x\n",
			    nh->nh_wirelen, *(int *)edst, *(short *)(edst+4),
			    *(int *)esrc, *(short *)(esrc+4));
			continue;
		}
		if (srcmask.a_mask && !srcmatch(sp))
			continue;
		if (dstmask.a_mask && !dstmatch(sp))
			continue;
		if (protomask.a_mask && !protomatch(sp))
			continue;
		if (lnthmask.a_mask && !lnthmatch(nh))
			continue;
		p->e_packets++;
		q->e_packets++;
		p->e_bytes += nh->nh_wirelen;
		q->e_bytes += nh->nh_wirelen;
		p->e_size[(nh->nh_wirelen - MINPACKETLEN)/BUCKETLNTH]++;

		type = *(short *)&sp->ts_eheader.ether_type;
		ip = (struct ip *)&sp->ts_ipovlyheader;
		/* 
		 * first compute dst, checking for broadcast specially
		 */
		edst = (char *)&sp->ts_eheader.ether_dhost;
		if (*(int *)edst == -1 && *(short *)(edst + 4) == -1) {
			p->e_bcast++;
			q->e_bcast++;
		}
		else if (type == ETHERTYPE_ARP)
			adddst(*(struct in_addr *)sp->ts_arpheader.arp_xtpa);
		else if (type == ETHERTYPE_IP)
			adddst(ip->ip_dst);

		if (type == ETHERTYPE_ARP) {
			addsrc(*(struct in_addr *)sp->ts_arpheader.arp_xspa);
			p->e_proto[ARPPROTO]++;
		}
		else if (type == ETHERTYPE_IP) {
			addsrc(ip->ip_src);
			switch(ip->ip_p) {
				case IPPROTO_ND:
					p->e_proto[NDPROTO]++;
					break;
				case IPPROTO_UDP:
					p->e_proto[UDPPROTO]++;
					break;
				case IPPROTO_TCP:
					p->e_proto[TCPPROTO]++;
					break;
				case IPPROTO_ICMP:
					p->e_proto[ICMPPROTO]++;
					break;
				default:
					p->e_proto[OTHERPROTO]++;
					break;
			}
		}
		else 
			p->e_proto[OTHERPROTO]++;
	}
}

initdevice()
{
	struct sockaddr_nit snit;
	struct nit_ioc nioc;
	
	if (if_fd < 0) {
		if_fd = socket(AF_NIT, SOCK_RAW, NITPROTO_RAW);
		if (if_fd < 0) {
			perror("nit socket");
			exit(-1);
		}

		snit.snit_family = AF_NIT;
		strncpy(snit.snit_ifname, device, NITIFSIZ);
		if (bind(if_fd, &snit, sizeof(snit)) != 0) {
			perror(device);
			exit (1);
		}

		bzero(&nioc, sizeof(nioc));
		nioc.nioc_bufspace = bufspace;
		nioc.nioc_chunksize = chunksize;
		nioc.nioc_typetomatch = NT_ALLTYPES;
		nioc.nioc_snaplen = snaplen;
		nioc.nioc_bufalign = sizeof (int);
		nioc.nioc_bufoffset = 0;
		nioc.nioc_flags = NF_PROMISC|NF_TIMEOUT;
		nioc.nioc_timeout.tv_sec = 1;
		nioc.nioc_timeout.tv_usec = 0;

		if (ioctl(if_fd, SIOCSNIT, &nioc) != 0) {
			perror("nit ioctl");
			exit(1);
		}
	}
}

deinitdevice()
{
	close(if_fd);
	if_fd = -1;
}

addsrc(inaddr)
	struct in_addr inaddr;
{	
	int addr, x;
	struct etherhmem *p, *q;
	
	addr = *(int *)&inaddr;
	x = addr &0xff;
	for (p = srchtable[x&0xff]; p != NULL; p = p->h_nxt) {
		if (p->h_addr == addr) {
			p->h_cnt++;
			return;
		}
	}
	p = (struct etherhmem *)malloc(sizeof(struct etherhmem));
	p->h_addr = addr;
	p->h_cnt = 1;
	p->h_nxt = srchtable[x];
	srchtable[x] = p;
}

adddst(inaddr)
	struct in_addr inaddr;
{	
	int addr, x;
	struct etherhmem *p, *q;
	
	addr = *(int *)&inaddr;
	x = addr &0xff;
	for (p = dsthtable[x&0xff]; p != NULL; p = p->h_nxt) {
		if (p->h_addr == addr) {
			p->h_cnt++;
			return;
		}
	}
	p = (struct etherhmem *)malloc(sizeof(struct etherhmem));
	p->h_addr = addr;
	p->h_cnt = 1;
	p->h_nxt = dsthtable[x];
	dsthtable[x] = p;
}

dstmatch(sp)
	struct sample *sp;
{
	int addr;
	
	switch (*(short *)&sp->ts_eheader.ether_type) {
		case ETHERTYPE_IP:
			addr = *(int *)
			       &((struct ip *)&sp->ts_ipovlyheader)->ip_dst;
			break;
		case ETHERTYPE_ARP:
			addr = *(int *)sp->ts_arpheader.arp_xtpa;
			break;
		default:
			return (0);
	}
	return ((addr & dstmask.a_mask)==(dstmask.a_addr & dstmask.a_mask));
}

srcmatch(sp)
	struct sample *sp;
{
	int addr;
	
	switch (*(short *)&sp->ts_eheader.ether_type) {
		case ETHERTYPE_IP:
			addr = *(int *)
			       &((struct ip *)&sp->ts_ipovlyheader)->ip_src;
			break;
		case ETHERTYPE_ARP:
			addr = *(int *)sp->ts_arpheader.arp_xspa;
			break;
		default:
			return (0);
	}
	return ((addr & srcmask.a_mask) == (srcmask.a_addr & srcmask.a_mask));
}

protomatch(sp)
	struct sample *sp;
{
	int proto;
	
	switch (*(short *)&sp->ts_eheader.ether_type) {
		case ETHERTYPE_IP:
			proto = *(char *)
			       &((struct ip *)&sp->ts_ipovlyheader)->ip_p;
			break;
		default:
			return (0);
	}
	return ((proto & protomask.a_mask) ==
	    (protomask.a_addr & protomask.a_mask));
}

/*
 * for size, a_addr is lowvalue, a_mask is high value
 */
lnthmatch(nh)
	struct nit_hdr *nh;
{
	int size;
	
	return (nh->nh_wirelen >= lnthmask.a_addr
	    && nh->nh_wirelen <= lnthmask.a_mask);
}

/* 
 * for debugging
 */
#if 0
printdata(table)
	struct etherhmem *table[];
#endif
printdata()
{
	int i;
	struct etherhmem *p;
	
	for (i = 0; i < HASHSIZE; i++) {
		for (p = srchtable[i]; p != NULL; p = p->h_nxt)
			printf("%x %d\n", p->h_addr, p->h_cnt);
	}
}

#ifndef lint
static  char sccsid[] = "@(#)etherfind.c 1.1 86/09/25 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <netinet/udp.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <net/nit.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/udp_var.h>
#include <netinet/ip_icmp.h>

#define BUFSPACE        4*8192
#define CHUNKSIZE       2048
#define SNAPLEN         (sizeof (struct sample))
#define NFDS 32

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

int nflag;			/* leave addresses as numbers */
int xflag;			/* dump headers in hex */
int pflag;			/* don't go promiscuous */
int cflag;			/* only print 'cnt' packets */

#define PROTOTABLESIZE 256
char *prototable[PROTOTABLESIZE];

#define ICMPTABLESIZE 256
char *icmptable[ICMPTABLESIZE];

struct anode {
	int (*F)();
	struct anode *L, *R;
};
struct anode *exlist;

char *getname();
void flushit();

#define IP ((struct ip *)&sp->ts_ipovlyheader)
#define	TYPE (*(u_short *)&sp->ts_eheader.ether_type)

struct sample *sp;	
u_short sp_ts_len;
int if_fd = -1;
char *device;
int bufspace = BUFSPACE;
int chunksize = CHUNKSIZE;
int snaplen = SNAPLEN;

main(argc, argv)
	char **argv;
{
	unsigned char buf[BUFSPACE];
	int cc, i, cnt;
	int on = 1;

	prototable[IPPROTO_ICMP] = "icmp";
	prototable[IPPROTO_TCP] = "tcp";
	prototable[IPPROTO_UDP] = "udp";
	prototable[IPPROTO_ND] = "nd";

	/* 
	 *
	 */

	while (argc > 1) {
		if (argv[1][0] != '-' || argv[1][1] == 0 || argv[1][2] != 0)
			break;
		switch(argv[1][1]) {
			case 'c':
				if (argc < 3)
					usage();
				cflag++;
				cnt = atoi(argv[2]);
				argc--;
				argv++;
				break;
			case 'n':
				nflag++;
				break;
			case 'x':
				xflag++;
				break;
			case 'p':
				pflag++;
				break;
			case 'u':
				setlinebuf(stdout);
				break;
			case 'i':
				if (argc < 3)
					usage();
				device = argv[2];
				argc--;
				argv++;
				break;
			default:
				goto done;
		}
		argc--;
		argv++;
	}
  done:
	if (device == NULL) {
		char buf[BUFSIZ];
		struct ifconf ifc;
		int s;

		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		    perror("etherfind: socket");
		    exit(1);
		}
		ifc.ifc_len = sizeof (buf);
		ifc.ifc_buf = buf;
		if (ioctl(s, SIOCGIFCONF, (char *)&ifc) < 0) {
			perror("etherfind: ioctl");
			exit(1);
		}
		device = ifc.ifc_req->ifr_name;
		close(s);
	}
	parseargs(argc, argv);
	initservarray();
	initicmptable();
	initdevice();
	signal(SIGINT, flushit);
	signal(SIGTERM, flushit);
	
  printf("                                           icmp type\n");
  printf(" lnth proto         source     destination  src port   dst port\n");

	i = 0;

refill_buffer:
	while ((cc = read(if_fd, buf, sizeof (buf))) > 0) {
		register unsigned char *bp, *bufstop;
		struct nit_hdr *nh;
		int datalen;

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
				fprintf(stderr,
				    "bad nit state %d\n", nh->nh_state);
				goto refill_buffer;
			}

			sp_ts_len = nh->nh_wirelen;
			if ((*exlist->F)(exlist)) {
				printit(sp, nh);
				i++;
				if (cflag && i >= cnt)
					exit(0);
			}
		}
	}
	perror("read");
	exit(-1);
}

/*
 *  convert host name to internet address
 */
nametoaddr(name)
	char *name;
{
	struct hostent *hp;

	hp = gethostbyname(name);
	if (hp == NULL) {
		fprintf(stderr, "%s unknown host name\n", name);
		return (0);
	}
	else
		return (*(int *)hp->h_addr);
}

nametonetaddr(name)
	char *name;
{
	struct netent *np;

	np = getnetbyname(name);
	if (np == NULL) {
		fprintf(stderr, "%s unknown net name\n", name);
		return (0);
	}
	else
		return (np->n_net);
}

/* 
 * convert a:b to a struct colon
 */
struct colon {
	int byte;
	unsigned value;
	char op;
} colon;

struct colon *
strtocolon(str)
{
	char *p, *ntoi();
	struct colon *colonp;
	
	colonp = (struct colon *)malloc(sizeof(colon));
	p = ntoi(str, &colonp->byte);
	colonp->op = *p++;
	ntoi(p, &colonp->value);
	if (colonp->byte >= SNAPLEN) {
		fprintf(stderr, "byte value is too large\n");
		exit(1);
	}
	return (colonp);
}

char *
ntoi(cp, valp)
	char *cp;
	int *valp;
{
	register val, base, c;
	
	val = 0; base = 10;
	if (*cp == '0')
		base = 8, cp++;
	if (*cp == 'x' || *cp == 'X')
		base = 16, cp++;
	while (c = *cp) {
		if (isdigit(c)) {
			val = (val * base) + (c - '0');
			cp++;
			continue;
		}
		if (base == 16 && isxdigit(c)) {
			val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
			cp++;
			continue;
		}
		break;
	}
	*valp = val;
	return (cp);
}

/*
 *  convert string (which may be host address or host name) to internet address
 */
stringtoaddr(str)
	char *str;
{
	if (isdigit(str[0]))
		return(inet_addr(str));
	else 
		return(nametoaddr(str));
}

struct addrpair {
	int addr1;
	int addr2;
};

/*
 *  convert two strings to an addrpair
 */
struct addrpair *
stringtoaddrpair(str1, str2)
	char *str1, *str2;
{
	struct addrpair *p;
	
	p = (struct addrpair *)malloc(sizeof(struct addrpair));
	p->addr1 = stringtoaddr(str1);
	p->addr2 = stringtoaddr(str2);
	return (p);
}

stringtonetaddr(str)
	char *str;
{
	if (isdigit(str[0]))
		return(inet_network(str));
	else 
		return(nametonetaddr(str));
}

stringtoport(str)
	char *str;
{
	struct servent *sp;
	
	if (isdigit(str[0]))
		return(atoi(str));
	else {
		sp = getservbyname(str, NULL);
		if (sp == NULL) {
			fprintf(stderr,
			 "etherfind: %s is not a valid port name or number\n",
			    str);
			exit(1);
		}
		return(sp->s_port);
	}
}

printit(sp, nh)
	struct sample *sp;
	struct nit_hdr *nh;
{
	int length, dst, src, proto, *intp, i, j, frag;
	char *getservname(), *p, fchar;
	struct in_addr inet_makeaddr();
	struct icmp *icmp;
	
	length = nh->nh_wirelen;
	if (TYPE != ETHERTYPE_IP && TYPE != ETHERTYPE_ARP
		&& TYPE != ETHERTYPE_REVARP) {
		printf("%4d type %04x\n", length, (TYPE)&0xffff);
		goto printx;
	}
	if (TYPE == ETHERTYPE_IP) {
		proto = IP->ip_p;
		frag = IP->ip_off & 0x1fff;
		fchar = (frag ? '*' : ' ');
		dst = *(int *)&IP->ip_dst;
		src = *(int *)&IP->ip_src;
		if (prototable[proto])
			printf("%c%4d %4s ", fchar, length, prototable[proto]);
		else
			printf("%c%4d 0x%02x ", fchar, length, proto);
	} else {
		p = (char *)&sp->ts_eheader.ether_dhost;
		dst = *(int *)sp->ts_arpheader.arp_xtpa;
		if (*(int *)p == -1 && *(short *)(p + 4) == -1)
			dst =
			   *(int *)&inet_makeaddr(inet_netof(dst), INADDR_ANY);
		src = *(int *)sp->ts_arpheader.arp_xspa;
		printf("%4d %4s ", length,
		    TYPE == ETHERTYPE_ARP ? "arp": "rarp");
	}
	if (nflag) {
		printf("%15x ", src);
		printf("%15x ", dst);
	}
	else {
		printf("%15.15s ", getname(src));
		printf("%15.15s ", getname(dst));
	}
	if (frag)
		printf("\n");
	else if (TYPE == ETHERTYPE_IP && proto == IPPROTO_ICMP) {
		icmp = (struct icmp *)&sp->ts_udpheader;
		printf("     %s\n", icmptable[icmp->icmp_type]);
	}
	else if (TYPE == ETHERTYPE_IP
	    && (proto == IPPROTO_TCP || proto == IPPROTO_UDP)) {
		if (nflag) {
			printf("%10d ", sp->ts_udpheader.uh_sport&0xffff);
			printf("%10d\n", sp->ts_udpheader.uh_dport&0xffff);
		}
		else {
			printf("%10.10s ",
			    getservname(sp->ts_udpheader.uh_sport, proto));
			printf("%10.10s\n",
			    getservname(sp->ts_udpheader.uh_dport, proto));
		}
	}
	else
		printf("\n");

    printx:
	if (xflag) {
		intp = (int *)sp;
		j = (sizeof(struct sample) - sizeof(u_short))/4;
		i = 0;
		while (j-- > 0) {
			i++;
			printf(" %08x", *intp++);
			if (i == 6) {
				i = 0;
				printf("\n");
			}
		}
		/* 
		 * Assume sample lnth is a multiple of 16
		 */
		if ((sizeof(struct sample) - sizeof(u_short)) % 4 != 0) {
			printf(" %04x", *(short *)intp);
			i++;
		}
		if (i != 0)
			printf("\n\n");
		else
			printf("\n");
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
		nioc.nioc_flags = pflag ? NF_TIMEOUT : NF_PROMISC|NF_TIMEOUT;
		nioc.nioc_timeout.tv_sec = 1;
		nioc.nioc_timeout.tv_usec = 0;

		if (ioctl(if_fd, SIOCSNIT, &nioc) != 0) {
			perror("nit ioctl");
			exit(1);
		}
	}
}

#define HASHNAMESIZE 256

struct hnamemem {
	int h_addr;
	char *h_name;
	struct hnamemem *h_nxt;
};
struct hnamemem *htable[HASHNAMESIZE];

char *
getname(addr)
{
	int x;
	struct hostent *hp;
	struct hnamemem *p;
	char buf[20];
	
	x = addr & 0xff;
	for (p = htable[x]; p != NULL; p = p->h_nxt) {
		if (p->h_addr == addr)
			return (p->h_name);
	}
	p = (struct hnamemem *)malloc(sizeof(struct hnamemem));
	p->h_addr = addr;
	p->h_nxt = htable[x];
	htable[x] = p;

	if (inet_lnaof(addr) == INADDR_ANY) {
		p->h_name = "broadcast";
		return (p->h_name);
	}
	hp = gethostbyaddr(&addr, sizeof(int), AF_INET);
	if (hp) {
		p->h_name = (char *)malloc(strlen(hp->h_name) + 1);
		strcpy(p->h_name, hp->h_name);
	}
	else {
		sprintf(buf, "0x%x", addr);
		p->h_name = (char *)malloc((strlen(buf)) + 1);
		strcpy(p->h_name, buf);
	}
	return (p->h_name);
}

#define TCP 0
#define UDP 1
#define NPROTOCOLS 2
#define MAXPORT 1024
char *names[NPROTOCOLS][MAXPORT];
#define NULLSTR ""

char *
getservname(port, proto)
{
	static char buf[10];
	int protonum;
	
	if (proto == IPPROTO_TCP)
		protonum = TCP;
	else if (proto == IPPROTO_UDP)
		protonum = UDP;
	else {
		sprintf(buf, "%d", port);
		return (buf);
	}
	if (port >= 0 && port < MAXPORT && names[protonum][port])
		return (names[protonum][port]);
	else {
		sprintf(buf, "%d", port);
		return (buf);
	}
}

initservarray()
{
	struct servent *sv;	
	int proto;

	while(sv = getservent()) {
		if (strcmp(sv->s_proto, "tcp") == 0)
			proto = TCP;
		else if (strcmp(sv->s_proto, "udp") == 0)
			proto = UDP;
		else
			continue;
		if (sv->s_port < 0 || sv->s_port >= MAXPORT)
			continue;
		names[proto][sv->s_port] =
		    (char *)malloc(strlen(sv->s_name) + 1);
		strcpy(names[proto][sv->s_port], sv->s_name);
	}
	endservent();
}

initicmptable()
{
	int i;
	char *p;
	
	icmptable[ICMP_ECHOREPLY] = "echo reply";
	icmptable[ICMP_UNREACH] = "dst unreachable";
	icmptable[ICMP_SOURCEQUENCH] = "src quench";
	icmptable[ICMP_REDIRECT] = "redirect";
	icmptable[ICMP_ECHO] = "echo";
	icmptable[ICMP_TIMXCEED] = "time exceeded";
	icmptable[ICMP_PARAMPROB] = "param problem";
	icmptable[ICMP_TSTAMP] = "timestamp";
	icmptable[ICMP_TSTAMPREPLY] = "timestamp reply";
	icmptable[ICMP_IREQ] = "info request";
	icmptable[ICMP_IREQREPLY] = "info reply";
	for (i = 0; i < ICMPTABLESIZE; i++) {
		if (icmptable[i] == NULL) {
			p = (char *)malloc(4);
			sprintf(p, "%d", i);
			icmptable[i] = p;
		}
	}
}

getproto(arg)
	char *arg;
{
	char **p;
	int protonum;
	
	if (isalpha(arg[0])) {
		protonum = -1;
		for (p = prototable; p  < prototable + PROTOTABLESIZE; p++) {
			if (*p && strcmp(arg, *p) == 0) {
				protonum = p - prototable;
				break;
			}
		}
		if (protonum == -1) {
			fprintf(stderr, "%s unknown protocol\n", arg);
			exit(1);
		}
		else
			return (protonum);
	}
	else
		return (atoi(arg));
}

#define EQ(x, y)	(strcmp(x, y)==0)

int	Randlast;

struct anode Node[100];
int Nn;  /* number of nodes */
int	Argc,
	Ai;
char	**Argv;
struct	anode	*exp(),
		*e1(),
		*e2(),
		*e3(),
		*mk();

char	*nxtarg();

parseargs(argc, argv)
	char *argv[];
{

	Argc = argc; Argv = argv;
	for(Ai = 1; Ai < (argc-1); ++Ai)
		if (*Argv[Ai] == '-' || EQ(Argv[Ai], "(") || EQ(Argv[Ai], "!"))
			break;
	if (!(exlist = exp())) { /* parse and compile the arguments */
		fprintf(stderr, "etherfind: parsing error\n");
		exit(1);
	}
}

/* compile time functions:  priority is  exp()<e1()<e2()<e3()  */

struct anode *exp()		/* parse ALTERNATION (-o)  */
{
	int or();
	register struct anode * p1;

	p1 = e1() /* get left operand */ ;
	if (EQ(nxtarg(), "-o")) {
		Randlast--;
		return(mk(or, p1, exp()));
	}
	else if (Ai <= Argc) --Ai;
	return(p1);
}

struct anode *e1() { /* parse CONCATENATION (formerly -a) */
	int and();
	register struct anode * p1;
	register char *a;

	p1 = e2();
	a = nxtarg();
	if (EQ(a, "-a")) {
And:
		Randlast--;
		return(mk(and, p1, e1()));
	} else if (EQ(a, "(") || EQ(a, "!") || (*a=='-' && !EQ(a, "-o"))) {
		--Ai;
		goto And;
	} else if (Ai <= Argc) --Ai;
	return(p1);
}

struct anode *e2()		/* parse NOT (!) */
{
	int not();

	if (Randlast) {
		fprintf(stderr, "etherfind: operand follows operand\n");
		exit(1);
	}
	Randlast++;
	if (EQ(nxtarg(), "!"))
		return(mk(not, e3(), (struct anode *)0));
	else if (Ai <= Argc) --Ai;
	return(e3());
}

struct anode *e3()		/* parse parens and predicates */
{
	int src(), dst(), srcnet(), dstnet(), less(), greater(), proto(),
	    srcport(), dstport(), byte(), between(),
	    broadcast(), arp(), rarp(), ip();
	struct anode *p1;
	register char *a, *b, *c, s;

	a = nxtarg();
	if (EQ(a, "(")) {
		Randlast--;
		p1 = exp();
		a = nxtarg();
		if (!EQ(a, ")")) goto err;
		return(p1);
	}
	else if (EQ(a, "-broadcast"))
		return(mk(broadcast, (struct anode *)0, (struct anode *)0));
	else if (EQ(a, "-arp"))
		return(mk(arp, (struct anode *)0, (struct anode *)0));
	else if (EQ(a, "-rarp"))
		return(mk(rarp, (struct anode *)0, (struct anode *)0));
	else if (EQ(a, "-ip"))
		return(mk(ip, (struct anode *)0, (struct anode *)0));
	b = nxtarg();
	s = *b;
	if (s=='+')
		b++;
	if (EQ(a, "-proto"))
		return(mk(proto, (struct anode *)getproto(b),
		    (struct anode *)s));
	else if (EQ(a, "-src"))
		return(mk(src, (struct anode *)stringtoaddr(b),
		    (struct anode *)s));
	else if (EQ(a, "-dst"))
		return(mk(dst, (struct anode *)stringtoaddr(b),
		    (struct anode *)s));
	else if (EQ(a, "-dstnet"))
		return(mk(dstnet, (struct anode *)stringtonetaddr(b),
		    (struct anode *)s));
	else if (EQ(a, "-srcnet"))
		return(mk(srcnet, (struct anode *)stringtonetaddr(b),
		    (struct anode *)s));
	else if (EQ(a, "-srcport"))
		return(mk(srcport, (struct anode *)stringtoport(b),
		    (struct anode *)s));
	else if (EQ(a, "-dstport"))
		return(mk(dstport, (struct anode *)stringtoport(b),
		    (struct anode *)s));
	else if (EQ(a, "-greater"))
		return(mk(greater, (struct anode *)atoi(b),
		    (struct anode *)s));
	else if (EQ(a, "-less"))
		return(mk(less, (struct anode *)atoi(b), (struct anode *)s));
	else if (EQ(a, "-byte"))
		return(mk(byte,(struct anode *)strtocolon(b),
		    (struct anode *)s));
	else if (EQ(a, "-between")) {
		c = nxtarg();
		return(mk(between, (struct anode *)stringtoaddrpair(b, c),
		    (struct anode *)s));
	}
err:	fprintf(stderr, "etherfind: bad option < %s >\n", a);
	usage();
	exit(1);
}

struct anode *mk(f, l, r)
	int (*f)();
	struct anode *l, *r;
{
	Node[Nn].F = f;
	Node[Nn].L = l;
	Node[Nn].R = r;
	return(&(Node[Nn++]));
}

char *nxtarg()			/* get next arg from command line */
{
	static strikes = 0;

	if (strikes==3) {
		fprintf(stderr, "etherfind: incomplete statement\n");
		usage();
		exit(1);
	}
	if (Ai>=Argc) {
		strikes++;
		Ai = Argc + 1;
		return("");
	}
	return(Argv[Ai++]);
}

/* execution time functions */
and(p)
	register struct anode *p;
{
	return(((*p->L->F)(p->L)) && ((*p->R->F)(p->R))?1:0);
}

or(p)
	register struct anode *p;
{
	return(((*p->L->F)(p->L)) || ((*p->R->F)(p->R))?1:0);
}

not(p)
	register struct anode *p;
{
	return( !((*p->L->F)(p->L)));
}

broadcast()
{
	char *edst;

	edst = (char *)&sp->ts_eheader.ether_dhost;
	return (*(int *)edst == -1 && *(short *)(edst + 4) == -1);
}

arp()
{
	return (TYPE == ETHERTYPE_ARP);
}

rarp()
{
	return (TYPE == ETHERTYPE_REVARP);
}

ip()
{
	return(TYPE == ETHERTYPE_IP);
}

proto(p)
	register struct { int f, u; } *p;
{
	return(TYPE == ETHERTYPE_IP && p->u == IP->ip_p);
}

dst(p)
	register struct { int f, u; } *p; 
{
	return((TYPE == ETHERTYPE_IP && p->u == *(int *)&IP->ip_dst)
	    || ((TYPE == ETHERTYPE_ARP || TYPE == ETHERTYPE_REVARP) &&
	    p->u == *(int *)sp->ts_arpheader.arp_xtpa));
}

src(p)
	register struct { int f, u; } *p; 
{
	return((TYPE == ETHERTYPE_IP && p->u == *(int *)&IP->ip_src)
	    || ((TYPE == ETHERTYPE_ARP || TYPE == ETHERTYPE_REVARP) &&
	    p->u == *(int *)sp->ts_arpheader.arp_xspa));
}

between(p)
	register struct { int f; struct addrpair *u; } *p; 
{
	int a,b;
	
	if (TYPE == ETHERTYPE_IP)
		return (((p->u->addr1 == *(int *)&IP->ip_src)
		    && (p->u->addr2 == *(int *)&IP->ip_dst))
		    || ((p->u->addr2 == *(int *)&IP->ip_src)
		    && (p->u->addr1 == *(int *)&IP->ip_dst)));
		    
	/* 
	 * else type is ETHERTYPE_ARP or REVARP
	 */
	return (((p->u->addr1 == *(int *)sp->ts_arpheader.arp_xspa)
	    && (p->u->addr2 == *(int *)sp->ts_arpheader.arp_xtpa))
	    || ((p->u->addr2 == *(int *)sp->ts_arpheader.arp_xspa)
	    && (p->u->addr1 == *(int *)sp->ts_arpheader.arp_xtpa)));
}

dstport(p)
	register struct { int f, u; } *p; 
{
	return(TYPE == ETHERTYPE_IP &&
	    (IP->ip_p == IPPROTO_UDP || IP->ip_p == IPPROTO_TCP)
	    && p->u == sp->ts_udpheader.uh_dport);
}

srcport(p)
	register struct { int f, u; } *p; 
{
	return(TYPE == ETHERTYPE_IP &&
	    (IP->ip_p == IPPROTO_UDP || IP->ip_p == IPPROTO_TCP)
	    && p->u == sp->ts_udpheader.uh_sport);
}

byte(p)
	register struct { int f; struct colon *u; } *p; 
{
	switch(p->u->op) {
	case '=':
		return (((u_char *)sp)[p->u->byte] == (u_char)p->u->value);
	case '<':
		return (((u_char *)sp)[p->u->byte] < (u_char)p->u->value);
	case '>':
		return (((u_char *)sp)[p->u->byte] > (u_char)p->u->value);
	case '&':
		return (((u_char *)sp)[p->u->byte] & (u_char)p->u->value);
	case '|':
		return (((u_char *)sp)[p->u->byte] | (u_char)p->u->value);
	}
}

srcnet(p)
	register struct { int f, u; } *p; 
{
	return((TYPE == ETHERTYPE_IP
	    && p->u == inet_netof(*(int *)&IP->ip_src))
	    || ((TYPE == ETHERTYPE_ARP || TYPE == ETHERTYPE_REVARP) &&
	    p->u == inet_netof(*(int *)sp->ts_arpheader.arp_xspa)));
}

dstnet(p)
	register struct { int f, u; } *p;
{
	return((TYPE == ETHERTYPE_IP
	    && p->u == inet_netof(*(int *)&IP->ip_dst))
	    || ((TYPE == ETHERTYPE_ARP || TYPE == ETHERTYPE_REVARP) &&
	    p->u == inet_netof(*(int *)sp->ts_arpheader.arp_xtpa)));
}

less(p)
	register struct { int f, u; } *p; 
{
	return(sp_ts_len <= p->u);
}

greater(p)
	register struct { int f, u; } *p; 
{
	return(sp_ts_len >= p->u);
}


static char *
any(cp, match)
	register char *cp;
	char *match;
{
	register char *mp, c;

	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return ((char *)0);
}

void
flushit()
{
	fflush(stdout);
	exit(1);
}

usage()
{
	fprintf(stderr,
"Usage: etherfind [-n] [-x] [-p] [-u] [-i interface] [-c cnt] options\n");
	fprintf(stderr,
"options are:\n");
	fprintf(stderr,
"       -broadcast -arp -rarp -ip\n");
	fprintf(stderr, 
"       -dst -src -dstnet -srcnet -less -greater -proto -srcport -dstport -byte\n");
	fprintf(stderr, 
"       -between\n");
}

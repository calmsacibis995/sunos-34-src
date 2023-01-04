#ifndef lint
static	char sccsid[] = "@(#)if.c 1.1 86/09/25 SMI"; /* from UCB 4.4 82/11/14 */
#endif

#include <sys/types.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>

#include <stdio.h>

extern	int kmem;
extern	int aflag;
extern	int tflag;
extern	int nflag;
extern	char *routename();

/*
 * Print a description of the network interfaces.
 */
intpr(interval, ifnetaddr, ifname)
	int interval;
	off_t ifnetaddr;
	char *ifname;
{
	struct ifnet ifnet;
	char name[16];

	if (ifnetaddr == 0) {
		printf("ifnet: symbol not defined\n");
		return;
	}
	if (interval) {
		sidewaysintpr(interval, ifnetaddr, ifname);
		return;
	}
	klseek(kmem, ifnetaddr, 0);
	read(kmem, &ifnetaddr, sizeof ifnetaddr);
	printf("%-5.5s %-5.5s %-10.10s  %-12.12s %-7.7s %-5.5s %-7.7s %-5.5s",
		"Name", "Mtu", "Net/Dest", "Address", "Ipkts", "Ierrs",
		"Opkts", "Oerrs");
	printf(" %-6.6s", "Collis");
	printf(" %-6.6s", "Queue");
	putchar('\n');
	while (ifnetaddr) {
		struct sockaddr_in *sin;
		register char *cp;
		char *index();
		struct in_addr in, inet_makeaddr();

		klseek(kmem, ifnetaddr, 0);
		read(kmem, &ifnet, sizeof ifnet);
		ifnetaddr = (off_t) ifnet.if_next;
		klseek(kmem, (int)ifnet.if_name, 0);
		read(kmem, name, 16);
		name[15] = '\0';
		cp = index(name, '\0');
		*cp++ = ifnet.if_unit + '0';
		*cp = '\0';
		if (ifname) {
			if (strcmp(ifname, name) != 0)
				continue;
		} else if ((ifnet.if_flags&IFF_UP) == 0) {
			if (!aflag)
				continue;
			*cp++ = '*';
			*cp = '\0';
		} else if (ifnet.if_addr.sa_family != AF_INET && !aflag)
			continue;
		printf("%-5.5s %-5d ", name, ifnet.if_mtu);
		if (ifnet.if_addr.sa_family == AF_INET) {
			if (ifnet.if_flags & IFF_POINTOPOINT) {
				sin = (struct sockaddr_in *)&ifnet.if_dstaddr;
				printf("%-10.10s  ", routename(sin->sin_addr));
			} else {
				in = inet_makeaddr(ifnet.if_net, INADDR_ANY);
				printf("%-10.10s  ", routename(in));
			}
			sin = (struct sockaddr_in *)&ifnet.if_addr;
			printf("%-12.12s ", routename(sin->sin_addr));
		} else {
			cp = " ";	/* XXX ??? */
			printf("%-10.10s  ", cp);
			printf("%-12.12s ", cp);
		}
		printf("%-7d %-5d %-7d %-5d %-6d",
		    ifnet.if_ipackets, ifnet.if_ierrors,
		    ifnet.if_opackets, ifnet.if_oerrors,
		    ifnet.if_collisions);
		printf(" %-6d", ifnet.if_snd.ifq_len);
		putchar('\n');
	}
}

#define	MAXIF	20
struct	iftot {
	char	ift_name[16];		/* interface name */
	int	ift_ip;			/* input packets */
	int	ift_ie;			/* input errors */
	int	ift_op;			/* output packets */
	int	ift_oe;			/* output errors */
	int	ift_co;			/* collisions */
} iftot[MAXIF];

/*
 * Print a running summary of interface statistics.
 * Repeat display every interval seconds, showing
 * statistics collected over that interval.  First
 * line printed at top of screen is always cumulative.
 */
sidewaysintpr(interval, off, ifname)
	int interval;
	off_t off;
	char *ifname;
{
	struct ifnet ifnet;
	off_t firstifnet;
	static char sobuf[BUFSIZ];
	register struct iftot *ip, *total;
	register int line;
	struct iftot *lastif, *sum, *interesting;
	int maxtraffic, traffic;

	setbuf(stdout, sobuf);
	klseek(kmem, off, 0);
	read(kmem, &firstifnet, sizeof (off_t));
	lastif = iftot;
	sum = iftot + MAXIF - 1;
	total = sum - 1;
	maxtraffic = 0, interesting = iftot;
	for (off = firstifnet, ip = iftot; off;) {
		char *cp;

		klseek(kmem, off, 0);
		read(kmem, &ifnet, sizeof ifnet);
		traffic = ifnet.if_ipackets + ifnet.if_opackets;
		if (traffic > maxtraffic)
			maxtraffic = traffic, interesting = ip;
		klseek(kmem, (int)ifnet.if_name, 0);
		ip->ift_name[0] = '(';
		read(kmem, ip->ift_name + 1, 15);
		ip->ift_name[15] = '\0';
		cp = index(ip->ift_name, '\0');
		sprintf(cp, "%d)", ifnet.if_unit);
		if (ifname &&
		    strncmp(ip->ift_name + 1, ifname, strlen(ifname)) == 0) {
			interesting = ip;
			maxtraffic = 0x7FFFFFFF;
		}
		ip++;
		if (ip >= iftot + MAXIF - 2)
			break;
		off = (off_t) ifnet.if_next;
	}
	lastif = ip;
banner:
	printf("    input   %-6.6s    output       ", interesting->ift_name);
	if (lastif - iftot > 0)
		printf("    input   (Total)    output       ");
	for (ip = iftot; ip < iftot + MAXIF; ip++) {
		ip->ift_ip = 0;
		ip->ift_ie = 0;
		ip->ift_op = 0;
		ip->ift_oe = 0;
		ip->ift_co = 0;
	}
	putchar('\n');
	printf("%-7.7s %-5.5s %-7.7s %-5.5s %-5.5s ",
		"packets", "errs", "packets", "errs", "colls");
	if (lastif - iftot > 0)
		printf("%-7.7s %-5.5s %-7.7s %-5.5s %-5.5s ",
			"packets", "errs", "packets", "errs", "colls");
	putchar('\n');
	fflush(stdout);
	line = 0;
loop:
	sum->ift_ip = 0;
	sum->ift_ie = 0;
	sum->ift_op = 0;
	sum->ift_oe = 0;
	sum->ift_co = 0;
	for (off = firstifnet, ip = iftot; off && ip < lastif; ip++) {
		klseek(kmem, off, 0);
		read(kmem, &ifnet, sizeof ifnet);
		if (ip == interesting)
			printf("%-7d %-5d %-7d %-5d %-5d ",
				ifnet.if_ipackets - ip->ift_ip,
				ifnet.if_ierrors - ip->ift_ie,
				ifnet.if_opackets - ip->ift_op,
				ifnet.if_oerrors - ip->ift_oe,
				ifnet.if_collisions - ip->ift_co);
		ip->ift_ip = ifnet.if_ipackets;
		ip->ift_ie = ifnet.if_ierrors;
		ip->ift_op = ifnet.if_opackets;
		ip->ift_oe = ifnet.if_oerrors;
		ip->ift_co = ifnet.if_collisions;
		sum->ift_ip += ip->ift_ip;
		sum->ift_ie += ip->ift_ie;
		sum->ift_op += ip->ift_op;
		sum->ift_oe += ip->ift_oe;
		sum->ift_co += ip->ift_co;
		off = (off_t) ifnet.if_next;
	}
	if (lastif - iftot > 0)
		printf("%-7d %-5d %-7d %-5d %-5d\n",
			sum->ift_ip - total->ift_ip,
			sum->ift_ie - total->ift_ie,
			sum->ift_op - total->ift_op,
			sum->ift_oe - total->ift_oe,
			sum->ift_co - total->ift_co);
	*total = *sum;
	fflush(stdout);
	line++;
	if (interval)
		sleep(interval);
	if (line == 21)
		goto banner;
	goto loop;
	/*NOTREACHED*/
}

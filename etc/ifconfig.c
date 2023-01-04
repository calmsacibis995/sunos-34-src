/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)ifconfig.c 1.4 86/11/10 SMI"; /* from UCB 4.18 5/22/86 */
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <net/if.h>

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>

char	*gethexnum();

struct	ifreq ifr;
struct	sockaddr_in sin = { AF_INET };
char	name[30];
int	s;

int	setifflags(), setifaddr();
int	setifnetmask(), setifbroadaddr();

#define	NEXTARG		0xffffff

struct	cmd {
	char	*c_name;
	int	c_parameter;		/* NEXTARG means next argv */
	int	(*c_func)();
} cmds[] = {
	{ "up",		IFF_UP,		setifflags } ,
	{ "down",	-IFF_UP,	setifflags },
	{ "trailers",	-IFF_NOTRAILERS,setifflags },
	{ "-trailers",	IFF_NOTRAILERS,	setifflags },
	{ "arp",	-IFF_NOARP,	setifflags },
	{ "-arp",	IFF_NOARP,	setifflags },
	{ "promisc",	IFF_PROMISC,	setifflags },
	{ "-promisc",	-IFF_PROMISC,	setifflags },
	{ "netmask",	NEXTARG,	setifnetmask },
	{ "broadcast",	NEXTARG,	setifbroadaddr },
	{ 0,		0,		setifaddr },
};

main(argc, argv)
	int argc;
	char *argv[];
{

	if (argc < 2) {
		fprintf(stderr, "usage: ifconfig interface\n%s%s%s%s",
		    "\t[ address ] [ up ] [ down ]\n",
		    "\t[ netmask mask ] [ broadcast address]\n",
		    "\t[ trailers | -trailers ]\n",
		    "\t[ arp | -arp ] [ promisc | -promisc]\n");
		exit(1);
	}
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("ifconfig: socket");
		exit(1);
	}
	argc--, argv++;
	strcpy(name, *argv);
	argc--, argv++;
	if (argc == 0) {
		status();
		exit(0);
	}
	while (argc > 0) {
		register struct cmd *p;

		for (p = cmds; p->c_name; p++)
			if (strcmp(*argv, p->c_name) == 0)
				break;
		if (p->c_func) {
			if (p->c_parameter == NEXTARG) {
				(*p->c_func)(argv[1]);
				argc--, argv++;
			} else
				(*p->c_func)(*argv, p->c_parameter);
		}
		argc--, argv++;
	}
	exit(0);
}

/*ARGSUSED*/
setifnetmask(addr)
	char *addr;
{
	in_getaddr(addr, &ifr.ifr_addr);
	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	if (ioctl(s, SIOCSIFNETMASK, (caddr_t)&ifr) < 0)
		Perror("ioctl (SIOCSIFNETMASK)");
}

/*ARGSUSED*/
setifbroadaddr(addr)
	char *addr;
{
	in_getaddr(addr, &ifr.ifr_addr);
	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	if (ioctl(s, SIOCSIFBRDADDR, (caddr_t)&ifr) < 0)
		Perror("ioctl (SIOCSIFBRDADDR)");
}

/*ARGSUSED*/
setifaddr(addr, param)
	char *addr;
	int param;
{

	in_getaddr(addr, &ifr.ifr_addr);
	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	if (ioctl(s, SIOCSIFADDR, (caddr_t)&ifr) < 0)
		Perror("ioctl (SIOCSIFADDR)");
}

setifflags(vname, value)
	char *vname;
	int value;
{

	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		Perror("ioctl (SIOCGIFFLAGS)");
	if (value < 0) {
		value = -value;
		ifr.ifr_flags &= ~value;
	} else
		ifr.ifr_flags |= value;
	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifr) < 0)
		Perror(vname);
}

#define	IFFBITS \
"\020\1UP\2BROADCAST\3DEBUG\4ROUTE\5POINTOPOINT\6NOTRAILERS\7RUNNING\10NOARP\11PROMISC"

/*
 * Print the status of the interface.
 */
status()
{
	struct sockaddr_in *sin;

	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR, (caddr_t)&ifr) < 0)
		Perror("ioctl (SIOCGIFADDR)");
	sin = (struct sockaddr_in *)&ifr.ifr_addr;
	printf("%s: %s ", name, inet_ntoa(sin->sin_addr));
	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	if (ioctl(s, SIOCGIFNETMASK, (caddr_t)&ifr) >= 0) {
		if (sin->sin_addr.s_addr)
		  printf("netmask %s ", inet_ntoa(sin->sin_addr));
	}
	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		Perror("ioctl (SIOCGIFFLAGS)");
	printb("flags", ifr.ifr_flags, IFFBITS); putchar('\n');
}

Perror(cmd)
	char *cmd;
{
	extern int errno;

	fprintf(stderr, "ifconfig: ");
	switch (errno) {

	case ENXIO:
		fprintf(stderr, "%s: ", cmd);
		fprintf(stderr, "no such interface\n");
		break;

	case EPERM:
		fprintf(stderr, "%s: permission denied\n", cmd);
		break;

	default:
		perror(cmd);
	}
	exit(1);
}

struct	in_addr inet_makeaddr();

in_getaddr(s, sa)
	char *s;
	struct sockaddr *sa;
{
	struct sockaddr_in *sin = (struct sockaddr_in *)sa;
	struct hostent *hp;
	struct netent *np;
	int val, n, o[8], *ip;
	char *p, *cp;

	bzero((caddr_t)sa, sizeof *sa);
	hp = gethostbyname(s);
	if (hp) {
		sin->sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, (char *)&sin->sin_addr, hp->h_length);
		return;
	}
	np = getnetbyname(s);
	if (np) {
		sin->sin_family = np->n_addrtype;
		sin->sin_addr = inet_makeaddr(np->n_net, INADDR_ANY);
		return;
	}
	sin->sin_family = AF_INET;
	val = inet_addr(s);
	if (val != -1) {
		sin->sin_addr.s_addr = val;
		return;
	}
	val = inet_network(s);
	if (val != -1) {
		sin->sin_addr = inet_makeaddr(val, INADDR_ANY);
		return;
	}
#ifdef notdef
	/* This code is too willing to accept garbage addresses... */
	n = sscanf(s, "%x:%x:%x:%x:%x:%x:%x:%x",
		&o[0], &o[1], &o[2], &o[3],
		&o[4], &o[5], &o[6], &o[7]);
	if (n > 0) {
		p = sa->sa_data;
		ip = &o[0];
		while (n--)
			*p++ = *ip++;
		sa->sa_family = AF_UNSPEC;
		return;
	}
#else notdef
	/*
	 * Dig out address fields by hand, checking that the
	 * address string consists of 1 or 2 digit hex numbers
	 * separated by colons.
	 */
	cp = s;
	for (n = 0; n < 8 && *cp; n++) {
		cp = gethexnum(cp, &o[n]);
		if (*cp && *cp != ':')
			break;
		/* The second clause handles trailing ":". */
		if (*cp && *(cp+1))
			cp++;
	}
	if (n > 0 && *cp == '\0') {
		p = sa->sa_data;
		ip = &o[0];
		while (n--)
			*p++ = *ip++;
		sa->sa_family = AF_UNSPEC;
		return;
	}
#endif notdef
	fprintf(stderr, "ifconfig: %s: bad address\n", s);
	exit(1);
}

/*
 * Print a value a la the %b format of the kernel's printf
 */
printb(s, v, bits)
	char *s;
	register char *bits;
	register unsigned short v;
{
	register int i, any = 0;
	register char c;

	if (bits && *bits == 8)
		printf("%s=%o", s, v);
	else
		printf("%s=%x", s, v);
	bits++;
	if (bits) {
		putchar('<');
		while (i = *bits++) {
			if (v & (1 << (i-1))) {
				if (any)
					putchar(',');
				any = 1;
				for (; (c = *bits) > 32; bits++)
					putchar(c);
			} else
				for (; *bits > 32; bits++)
					;
		}
		putchar('>');
	}
}

/*
 * Convert at most two digits of a string representing a
 * hex number to its value.  Return a pointer to the character
 * immediately following the last one converted.
 */
char *
gethexnum(inp, valp)
	char	*inp;
	int	*valp;
{
	register char	c;
	register int	i;

	*valp = 0;

	for (i = 0; i < 2; i++) {
		if (!inp)
			return (inp);
		c = *inp;
		if (!isascii(c) || !isxdigit(c))
			return (inp);

		/* Have a valid digit. */
		inp++;
		if (isdigit(c))
			*valp = (*valp << 4) + c - '0';
		else if (isupper(c))
			*valp = (*valp << 4) + 10 + c - 'A';
		else
			*valp = (*valp << 4) + 10 + c - 'a';
	}
	return (inp);
}

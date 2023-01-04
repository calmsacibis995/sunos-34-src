/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)trace.c 1.1 86/09/24 SMI";  /* from UCB 5.1 6/4/85 */
#endif not lint

#include <sys/param.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <protocols/routed.h>

struct	sockaddr_in myaddr = { AF_INET, IPPORT_RESERVED-1 };
char	packet[MAXPACKETSIZE];

main(argc, argv)
	int argc;
	char *argv[];
{
	int size, s;
	struct sockaddr from;
	struct sockaddr_in router;
	register struct rip *msg = (struct rip *)packet;
	struct hostent *hp;
	struct servent *sp;
	
	if (argc < 3) {
usage:
		printf("usage: trace cmd machines,\n");
		printf("cmd either \"on filename\", or \"off\"\n");
		exit(1);
	}
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("socket");
		exit(2);
	}
#ifdef vax || pdp11
	myaddr.sin_port = htons(myaddr.sin_port);
#endif
	if (bind(s, &myaddr, sizeof(myaddr)) < 0) {
		perror("bind");
		exit(2);
	}

	argv++, argc--;
	msg->rip_cmd = strcmp(*argv, "on") == 0 ?
		RIPCMD_TRACEON : RIPCMD_TRACEOFF;
	msg->rip_vers = RIPVERSION;
	argv++, argc--;
	size = sizeof (int);
	if (msg->rip_cmd == RIPCMD_TRACEON) {
		strcpy(msg->rip_tracefile, *argv);
		size += strlen(*argv);
		argv++, argc--;
	}
	if (argc == 0)
		goto usage;
	bzero((char *)&router, sizeof (router));
	router.sin_family = AF_INET;
	sp = getservbyname("router", "udp");
	if (sp == 0) {
		printf("udp/router: service unknown\n");
		exit(1);
	}
	router.sin_port = sp->s_port;
	while (argc > 0) {
		hp = gethostbyname(*argv);
		if (hp == 0) {
			printf("%s: unknown\n", *argv);
			continue;
		}
		bcopy(hp->h_addr, &router.sin_addr, hp->h_length);
		if (sendto(s, packet, size, 0, &router, sizeof(router)) < 0)
			perror(*argv);
		argv++, argc--;
	}
}

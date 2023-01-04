#include <sys/types.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <nlist.h>
#include <setjmp.h>
#include <signal.h>

#define	CTOB(x)			(((u_long)(x)) * pagesize)
#define BTOC(x)			(((u_long)(x)) / pagesize)
#define PADDR(page, addr)	(CTOB(page) + ((u_long)addr & (pagesize-1)))



int	signalgot;

static char     sccsid[] = "@(#)probe.c 1.1 9/25/86 Copyright 1985 Sun Micro";

main(argc, argv)
char *argv[];
{
	extern u_char	*valloc();
	register u_char	c, *pageaddr;
	register	pagesize = getpagesize();
	int		fd, memtype, memaddr;
	int		bus(), segv(), intr(), hup();
	
	if (argc != 3){
		printf("Usage: %s (0,1 = P2mem,io  2,3 = mbmem,io) ADDR \n",
			*argv);
		exit(1);
	} else {
		memtype = atoi(argv[1]);
		sscanf(argv[2], "%x", &memaddr);
		printf("checking type %d addr 0x%x\n", memtype, memaddr);
	}
		
	switch(memtype){
		case 0:
			if ((fd = open("/dev/mem",  O_RDONLY)) < 0) {
				printf("probe: /dev/mem bit it\n");
				exit(4);
			}
			break;
		case 1:
			printf("You can't map on board I/O!!\n");
			exit(5);
			break;
		case 2:
			if ((fd = open("/dev/mbmem",  O_RDONLY)) < 0) {
				printf("probe: /dev/mbmem bit it\n");
				exit(6);
			}
			break;
		case 3:
			if ((fd = open("/dev/mbio",  O_RDONLY)) < 0) {
				printf("probe: /dev/mbio bit it\n");
				exit(7);
			}
			break;
		default:
			printf("probe: no such type as %d\n",
				memtype);
			exit(8);
			break;
	}
	if ((pageaddr = valloc(pagesize)) == 0) {
		printf("probe: valloc bit it\n");
		exit(9);
	}

	signal(SIGBUS, bus);
	signal(SIGSEGV, segv);
	signal(SIGINT, intr);
	signal(SIGHUP, hup);

	if (mmap(pageaddr, pagesize,
		PROT_READ, MAP_SHARED, fd, CTOB(BTOC(memaddr))) < 0){
		printf("probe: mmap bit it");
		exit(10);
	}
	c = pageaddr[memaddr % pagesize];
	printf("access went ok\n");
	exit(0);
}

bus()
{
	signalgot = SIGBUS;
	printf("probe: sigbus got us\n");
	exit(21);
}

segv()
{
	signalgot = SIGSEGV;
	printf("probe: sigsegv got us\n");
	exit(22);
}

intr()
{
	signalgot = SIGINT;
	printf("probe: sigint got us\n");
	exit(23);
}

hup()
{
	signalgot = SIGHUP;
	printf("probe: sighup got us\n");
	exit(24);
}

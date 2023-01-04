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

static char     sccsid[] = "@(#)skyprobe.c 1.1 9/25/86 Copyright 1985 Sun Micro";

/*	systype returns 0 for a Multibus (TM Intel) or 1 for a VMEbus system */

systype()

{

    return((gethostid() & 0xff000000) == 0x02000000);
}	

main(argc, argv)
char *argv[];
{
	extern u_char	*valloc();
	register u_char	*pageaddr;
	register	pagesize = getpagesize();
	int		fd, memtype, memaddr;
	int		systype(), bus(), segv(), intr(), hup();
	int		mbio = 0, vmeio = 1;
	int             debug;                  /* debug switch */
	int		b;

	debug = 1;

	if (argc == 2){
	   if (argv[1][0] == 't') exit(systype());
        }
	     
  
	if (argc != 3){
		printf("Usage: %s (t = bus type?/0,1 = P2mem,io/2,3 = mbmem,io/4 = vmemem,io) ADDR \n", *argv);
		exit(1);
	} else {
		memtype = atoi(argv[1]);
		sscanf(argv[2], "%x", &memaddr);
		printf("skyprobe: checking type %d addr 0x%x\n", memtype, memaddr);
	}
		
	switch(memtype){
		case 0:
			if ((fd = open("/dev/mem",  O_RDONLY)) < 0) {
				printf("skyprobe: /dev/mem bit it\n");
				exit(4);
			}
			break;
		case 1:
			printf("skyprobe: You can't map on board I/O!!\n");
			exit(5);
			break;
		case 2:
			if ((fd = open("/dev/mbmem",  O_RDONLY)) < 0) {
				printf("skyprobe: /dev/mbmem bit it\n");
				exit(6);
			}
			break;
		case 3:
			if (systype() == mbio) {
			  if ((fd = open("/dev/mbio",  O_RDONLY)) < 0) {
				printf("skyprobe: /dev/mbio bit it\n");
				exit(7);
 			  }
			}
			else exit(8);
			break;
		case 4:
			if (systype() == vmeio) {
			  if ((fd = open("/dev/vme16",  O_RDONLY)) < 0) {
				printf("skyprobe: /dev/vme16 bit it\n");
				exit(9);
 			  }
			}
			else exit(10);

			if (debug) printf("skyprobe: past open\n");

			break;
		default:
			printf("skyprobe: no such type as %d\n",
				memtype);
			exit(11);
			break;
	}
	if ((pageaddr = valloc(pagesize)) == 0) {
		printf("skyprobe: valloc bit it\n");
		exit(12);
	}

	if (debug) printf("skyprobe: past valloc\n");

	signal(SIGBUS, bus);
	signal(SIGSEGV, segv);
	signal(SIGINT, intr);
	signal(SIGHUP, hup);

	if (debug) printf("skyprobe: past signal setup\n");

	if (mmap(pageaddr, pagesize,
		PROT_READ, MAP_SHARED, fd, CTOB(BTOC(memaddr))) < 0){
		printf("skyprobe: mmap bit it, addr 0x%x\n", memaddr);
		exit(13);
	}

	if (debug) printf("skyprobe: CTOB(BTOC(memaddr)= 0x%x\n", CTOB(BTOC(memaddr)));

	if (debug) printf("skyprobe: past mmap\n");
	
	if (debug) printf("skyprobe: [memaddr % pagesize] = 0x%x\n", (memaddr % pagesize));

	if (debug) printf("Command register: %x\n", *(u_short *)pageaddr);
 
	b = *(u_short *)pageaddr;
        
	if (debug) printf("skyprobe: past b= %x\n", b);

	printf("skyprobe: access went ok\n");
	exit(0);
}

bus()
{
	signalgot = SIGBUS;
	printf("skyprobe: sigbus got us\n");
	exit(21);
}

segv()
{
	signalgot = SIGSEGV;
	printf("skyprobe: sigsegv got us\n");
	exit(22);
}

intr()
{
	signalgot = SIGINT;
	printf("skyprobe: sigint got us\n");
	exit(23);
}

hup()
{
	signalgot = SIGHUP;
	printf("skyprobe: sighup got us\n");
	exit(24);
}

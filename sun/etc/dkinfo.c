#ifndef lint
static	char sccsid[] = "@(#)dkinfo.c 1.5 86/12/15 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Report information about a disk's geometry and partitioning
 *
 * Usage: dkinfo XXN
 * where XX is controller name (ip, xy, etc) and N is disk number.
 * or dkinfo rawname where rawname is a character special file for a disk.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <sun/dklabel.h>
#include <sun/dkio.h>

struct dk_geom g;
struct dk_map p;
struct dk_info inf;

main(argc, argv)
	char **argv;
{
	while (--argc)
		dk(*++argv);
}

dk(name)
	char *name;
{
	char nbuf[100];
	struct stat st;
	int fd,	c, found, nparts;

	sprintf(nbuf, "/dev/r%s", name);
	if (stat(nbuf, &st) == 0) {
		fd = open(nbuf,	0);
		if (fd < 0) {
			perror(nbuf);
			return;
		}
		info(fd);
		geom(fd);
		if (parts(0, fd) == 0)
			printf("Not a valid partition.\n");
		close(fd);
		return;
	}

	found =	0;
	nparts = 0;
	for (c = 'a'; c	<= 'h';	c++) {
		sprintf(nbuf, "/dev/r%s%c", name, c);
		if (stat(nbuf, &st))
			continue;
		fd = open(nbuf,	0);
		if (fd < 0) {
			nbuf[0]	= c;
			nbuf[1]	= 0;
			perror(nbuf);
			continue;
		}
		if (!found) {
			printf("%s: ", name);
			info(fd);
			geom(fd);
			found =	1;
		}
		nparts += parts(c, fd);
		close(fd);
	}
	if (!found)
		printf("%s: no such disk\n", name);
	else if	(nparts	== 0)
		printf("%s: no valid partitions\n", name);
}

char *ctlrname[] = {
	"Unknown",
	"SMD-2180",
	"WDC-2880",
	"SMD-2181",
	"Unknown",			/* used to be xy440 */
	"DSD 5215",
	"Xylogics 450/451",
	"Adaptec ACB4000",
	"Emulex MD21",
};
#define	NCTLR	(sizeof (ctlrname)/sizeof (ctlrname[0]))
#define CTLRNAME(n)	ctlrname[(n) >= NCTLR ? 0 : (n)]

info(fd)
{
	if (ioctl(fd, DKIOCINFO, &inf) == 0) {
		printf("%s controller at addr %x, unit # %d\n",
			CTLRNAME(inf.dki_ctype), inf.dki_ctlr, inf.dki_unit);
	} else {
		printf("can't get disk ident info\n");
	}
}

geom(fd)
{
	if (ioctl(fd, DKIOCGGEOM, &g) == 0) {
		printf("%d cylinders", g.dkg_ncyl);
		if (g.dkg_bcyl)
			printf(" (base %d)", g.dkg_bcyl);
		printf(" %d heads", g.dkg_nhead);
		if (g.dkg_bhead)
			printf(" (base %d)", g.dkg_bhead);
		printf(" %d sectors/track\n", g.dkg_nsect);
	} else {
		printf("can't remember my geometry\n");
	}
}

parts(pa, fd)
{
	int n;

	if (ioctl(fd, DKIOCGPART, &p) == 0) {
		if (p.dkl_nblk == 0)
			return (0);
		if (pa)	printf("%c: ", pa);
		printf("%d sectors ", p.dkl_nblk);
		printf("(%d cyls", p.dkl_nblk/(g.dkg_nhead*g.dkg_nsect));
		n = (p.dkl_nblk % (g.dkg_nhead*g.dkg_nsect)) / g.dkg_nsect;
		if (n) printf(", %d tracks", n);
		n = p.dkl_nblk % g.dkg_nsect;
		if (n) printf(", %d sectors", n);
		printf(")\n");
		if (pa)	printf("   ");
		printf("starting cylinder %d\n", p.dkl_cylno);
		return p.dkl_nblk ? 1 :	0;
	} else {
		printf("can't get partition info\n");
		return (0);
	}
}

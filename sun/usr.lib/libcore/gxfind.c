#ifndef lint
static char sccsid[] = "@(#)gxfind.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
#include <framebuf.h>
#include <sys/mman.h>

int GXBase;
static int GXFile;
static int allocp;
#define	GXSIZE	GXaddrRange
#define	GXALIGN	getpagesize()

_core_GXopen()
{
	int p;

	if (GXBase)
		return (GXBase);
	GXFile = open("/dev/console", 1);
	if (GXFile < 0)	{
		/* perror("gxfind"); */
		return(0);
	}
	p = allocp = (int)malloc(GXSIZE+GXALIGN);
	p = (p + GXALIGN -1) & ~(GXALIGN-1);
	if (mmap(p, GXSIZE, PROT_WRITE,	MAP_SHARED, GXFile, 0))
		/* perror("gxfind mmap") */ ;
	GXBase = p;
	return(p);
}

static mmap(addr, len,	prot, share, fd, pos)
{
	return(syscall(64+7, addr, len,	prot, share, fd, pos));
}


_core_GXclose()
	{
	close(GXFile);
	free(allocp);
	GXBase=0;
	}

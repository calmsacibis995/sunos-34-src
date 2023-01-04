#ifndef lint
static	char sccsid[] = "@(#)skyinit.c 1.1 86/09/24 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <sys/mman.h>
#define SKY "/dev/sky"

char * _skybase;
static int skyfd;
extern int errno;
extern char *valloc();

_skyinit()
{
    /* 
     * map sky ffp into user address space
     */
    register ps;
    register short *stcreg;
    int saveerrno = errno;

    if (_skybase) return 1; /* already done */
    ps = getpagesize();
    skyfd = open( SKY, 2);
    if (skyfd<0){
	errno = saveerrno;
	return 0;
    }
    _skybase=valloc( ps );
    if (_skybase == 0){
	close( skyfd );
	errno = saveerrno;
	return 0;
    }
    errno = 0;
    mmap( _skybase, ps, PROT_READ|PROT_WRITE, MAP_SHARED, skyfd, 0 );
    if (errno){
	close( skyfd );
	_skybase = 0; /* throw away valloc'ed space */
	errno = saveerrno;
	return 0;
    }
    
    _skybase += 4; /* point at the data port */
    
    errno = saveerrno;
    return 1;
}

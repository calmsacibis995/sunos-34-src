#ifdef sccsid
static  char sccsid[] = "@(#)sinitfp.c 1.1 86/09/24 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/mman.h>
#include "fpcrttypes.h"
#define SKY "/dev/sky"

       int _sky_fd;
extern int errno;
extern char *valloc();

sinitfp_()
{
    /* 
     * map sky ffp into user address space
     */
    register ps;
    register short *stcreg;
    int saveerrno = errno;

    if (fp_state_skyffp != fp_unknown) 
	{
	if (fp_state_skyffp==fp_enabled) fp_switch = fp_skyffp ;
	return((fp_state_skyffp==fp_enabled) ? 1 : 0); /* already done */
    	}
    ps = getpagesize();
    _sky_fd = open( SKY, 2);
    if (_sky_fd<0){
	errno = saveerrno;
    	fp_state_skyffp = fp_absent ;
	return 0;
	}
    _skybase= valloc( ps );
    if (_skybase == 0){
	close( _sky_fd );
	errno = saveerrno;
    	fp_state_skyffp = fp_absent ;
	return 0;
	}
    errno = 0;
    mmap( _skybase, ps, PROT_READ|PROT_WRITE, MAP_SHARED, _sky_fd, 0 );
    if (errno){
	close( _sky_fd );
	_skybase = 0; /* throw away valloc'ed space */
	errno = saveerrno;
	fp_state_skyffp = fp_absent ;
	return 0;
    }
    
    _skybase += 4; /* point at the data port */
    
    errno = saveerrno;
    fp_state_skyffp = fp_enabled ;
    fp_switch = fp_skyffp ;
    return 1;
}

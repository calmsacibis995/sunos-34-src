/*      @(#)msio.h 1.1 86/09/25 SMI      */
#ifndef MSIO
#define MSIO

/*
 * msio.h:	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

typedef struct {
    int             jitter_thresh;
    int             speed_law;
    int             speed_limit;
}               Ms_parms;

#define	MSIOGETPARMS	_IOR(m, 2, Ms_parms) /*  get / set jitter, speed  */
#define	MSIOSETPARMS	_IOW(m, 3, Ms_parms) /*  law, or speed limit	   */

#endif MSIO

/*	@(#)sasun.h 1.3 83/09/16 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Constants for standalone I/O (bootstrap) code
 */
#include "../h/s2addrs.h"
#define	DEF_MBMEM_VA	MBMEM_BASE
#define	DEF_MBIO_VA	MBIO_BASE

#define	LOADADDR	0x4000
#define	BBSIZE		8192		/* boot block size (from fs.h) */
#define	DEV_BSIZE	512		/* manifest */
#define	MAX(a,b)	(((a)>(b))? (a): (b))
#define	DELAY(n)	{ register int N = (n)>>1; while (--N > 0); }

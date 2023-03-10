#ifndef lint
static	char sccsid[] = "@(#)mallopt.c 1.1 86/09/24";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "mallint.h"
#include <errno.h>

#define ALIGNSZ WORDSIZE

/*
 * mallopt -- System V-compatible malloc "optimizer"
 */
mallopt(cmd, value)
int cmd, value;
{
	if (__mallinfo.smblks != 0)
		return(-1);		/* small block has been allocated */

	switch (cmd) {
	case M_MXFAST:		/* small block size */
		if (value < 0)
			return(-1);
		__mallinfo.mxfast = value;
		break;

	case M_NLBLKS:		/* # small blocks per holding block */
		if (value <= 0)
			return(-1);
		__mallinfo.nlblks = value;
		break;

	case M_GRAIN:		/* small block rounding factor */
		if (value <= 0)
			return(-1);
		/* round up to multiple of minimum alignment */
		__mallinfo.grain = roundup(value, ALIGNSZ);
		break;

	case M_KEEP:		/* Sun algorithm always preserves data */
		break;

	default:
		return(-1);
	}

	/* make sure that everything is consistent */
	__mallinfo.mxfast = roundup(__mallinfo.mxfast, __mallinfo.grain);

	return(0);
}


/*
 * mallinfo -- System V-compatible malloc information reporter
 */
struct mallinfo
mallinfo()
{
	struct mallinfo mi;

	mi = __mallinfo;
	mi.uordblks = mi.uordbytes - (mi.allocated * sizeof(uint));
	mi.fordblks = mi.arena - (mi.treeoverhead + mi.uordblks +
					    (mi.ordblks * sizeof(uint)));
	return(mi);
}

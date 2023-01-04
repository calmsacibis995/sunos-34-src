/*	@(#)pginit.h 1.3 83/09/16 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * pginit.h -- structure used to define large continuous page map areas
 */
struct pginit {
	char		*pi_addr;	/* Virtual address to map at */
	short		pi_incr;	/* page # increment each time */
	struct pgmapent	pi_pm;		/* The first page map entry */
};

#define PGINITEND	0x8000		/* incr value to end table */


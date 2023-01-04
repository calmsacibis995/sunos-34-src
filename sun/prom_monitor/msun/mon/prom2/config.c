/*
 * @(#)config.c 1.17 84/11/29 Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "../h/bootparam.h"
#include "../h/s2addrs.h"
#include "../h/globram.h"
#include "../h/diag.h"

extern struct boottab boottab[];		/* boot.c */

showconfig()
{
	register struct boottab *tp;

	/*
	 * Print a message before we try to touch bus, since we'll hang
	 * forever if we never get bus priority.
	 */
#ifdef VME
	printf("Probing I/O bus");
#else  VME
	printf("Probing Multibus");
#endif VME
	(void) peek(MBMEM_BASE);
	printf(":");
	
#ifdef DES
	if (gp->g_diag_state.ds_damages & (1 << DESFOUND))
		printf(" des"); 
#endif DES

	for (tp = boottab; tp->b_dev[0]; tp++) {
		if (-1 != (*tp->b_probe)()) {
			printf(" %c%c", tp->b_dev[0], tp->b_dev[1]);
		}
	}
	printf ("\n");
}

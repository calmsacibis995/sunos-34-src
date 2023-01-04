#ifndef lint
static	char sccsid[] = "@(#)setbrk.c 1.1 86/09/24 SMI"; /* from S5R2 1.4 */
#endif

/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */

#include	"defs.h"

char 	*sbrk();

char*
setbrk(incr)
{

	register char *a = sbrk(incr);

	brkend = a + incr;
	return(a);
}

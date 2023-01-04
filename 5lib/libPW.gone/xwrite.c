#ifndef lint
static	char sccsid[] = "@(#)xwrite.c 1.1 86/09/24 SMI"; /* from S5R2 3.3 */
#endif

# include "errno.h"

/*
	Interface to write which handles
	all error conditions.
	Returns number of bytes written on success,
	returns fatal(<mesg>) on failure.
*/

xwrite(fildes,buffer,nbytes)
char *buffer;
{
	register int n;

	if (nbytes>0 && (n=write(fildes,buffer,nbytes))!=nbytes)
		n = xmsg("","xwrite");
	return(n);
}

#ifndef lint
static	char sccsid[] = "@(#)xpipe.c 1.1 86/09/24 SMI"; /* from S5R2 3.1 */
#endif

/*
	Interface to pipe(II) which handles all error conditions.
	Returns 0 on success,
	fatal() on failure.
*/

xpipe(t)
int *t;
{
	static char p[]="pipe";

	if (pipe(t) == 0)
		return(0);
	return(xmsg(p,p));
}

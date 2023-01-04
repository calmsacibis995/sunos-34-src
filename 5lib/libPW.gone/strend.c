#ifndef lint
static	char sccsid[] = "@(#)strend.c 1.1 86/09/24 SMI"; /* from S5R2 3.1 */
#endif

char *strend(p)
register char *p;
{
	while (*p++)
		;
	return(--p);
}

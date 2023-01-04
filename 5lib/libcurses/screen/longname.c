#ifndef lint
static	char sccsid[] = "@(#)longname.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

/*
 *	This routine returns the long name of the terminal.
 */
char *
longname()
{
	register char	*cp;
	extern char ttytype[];

	for (cp=ttytype; *cp++; )		/* Go to end of string */
		;
	while (*--cp != '|' && cp>=ttytype)	/* Back up to | or beginning */
		;
	return ++cp;
}

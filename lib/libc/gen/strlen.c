#ifndef lint
static	char sccsid[] = "@(#)strlen.c 1.1 86/09/24 SMI"; /* from UCB 4.1 80/12/21 */
#endif

/*
 * Returns the number of
 * non-NULL bytes in string argument.
 */

strlen(s)
	register char *s;
{
	register int n;

	n = 0;
	while (*s++)
		n++;
	return (n);
}

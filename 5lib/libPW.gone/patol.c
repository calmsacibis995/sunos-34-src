#ifndef lint
static	char sccsid[] = "@(#)patol.c 1.1 86/09/24 SMI"; /* from S5R2 3.1 */
#endif

/*
	Function to convert ascii string to long.  Converts
	positive numbers only.  Returns -1 if non-numeric
	character encountered.
*/

long
patol(s)
register char *s;
{
	long i;

	i = 0;
	while (*s >= '0' && *s <= '9')
		i = 10*i + *s++ - '0';

	if (*s)
		return(-1);
	return(i);
}

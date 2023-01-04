#ifndef lint
static	char sccsid[] = "@(#)anystr.c 1.1 86/09/24 SMI"; /* from S5R2 3.1 */
#endif

# include	"sys/types.h"
# include	"macros.h"

/*
	This routine returns the position of the first character
	in s1 that matches any character in s2.
*/

anystr(s1, s2)
char *s1, *s2;
{
	register int i = -1;
	register int c;

	while (c = s1[++i])
		if (any(c, s2))
			return(i);
	return(-1);
}

#ifndef lint
static	char sccsid[] = "@(#)tmpnam.c 1.1 86/09/24 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>

extern char *mktemp(), *strcpy(), *strcat();
static char str[L_tmpnam], seed[] = { 'a', 'a', 'a', '\0' };

char *
tmpnam(s)
char	*s;
{
	register char *p, *q;

	p = (s == NULL)? str: s;
	(void) strcpy(p, P_tmpdir);
	(void) strcat(p, seed);
	(void) strcat(p, "XXXXXX");

	q = seed;
	while(*q == 'z')
		*q++ = 'a';
	++*q;

	(void) mktemp(p);
	return(p);
}

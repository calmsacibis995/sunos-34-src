#ifndef lint
static	char sccsid[] = "@(#)tolower.c 1.1 86/09/24 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
/*
 * If arg is upper-case, return the lower-case, else return the arg.
 */

int
tolower(c)
register int c;
{
	if(c >= 'A' && c <= 'Z')
		c -= 'A' - 'a';
	return(c);
}

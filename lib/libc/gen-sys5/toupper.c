#ifndef lint
static	char sccsid[] = "@(#)toupper.c 1.1 86/09/24 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
/*
 * If arg is lower-case, return upper-case, otherwise return arg.
 */

int
toupper(c)
register int c;
{
	if(c >= 'a' && c <= 'z')
		c += 'A' - 'a';
	return(c);
}

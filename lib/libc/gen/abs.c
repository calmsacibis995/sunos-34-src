#ifndef lint
static	char sccsid[] = "@(#)abs.c 1.1 86/09/24 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/

int
abs(arg)
register int arg;
{
	return (arg >= 0 ? arg : -arg);
}

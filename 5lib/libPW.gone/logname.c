#ifndef lint
static	char sccsid[] = "@(#)logname.c 1.1 86/09/24 SMI"; /* from S5R2 */
#endif

char *
logname()
{
	extern char *getenv();

	return(getenv("LOGNAME"));
}

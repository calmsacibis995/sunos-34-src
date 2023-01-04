#ifndef lint
static	char sccsid[] = "@(#)tmpnam.c 1.1 86/09/24 SMI"; /* from UCB 4.1 80/12/21 */
#endif

char *tmpnam(s)
char *s;
{
	static seed;

	sprintf(s, "temp.%d.%d", getpid(), seed++);
	return(s);
}

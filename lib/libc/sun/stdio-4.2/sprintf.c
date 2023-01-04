#ifndef lint
static	char sccsid[] = "@(#)sprintf.c 1.1 86/09/24 SMI"; /* from S5R2 1.5 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#include <values.h>

extern int _doprnt();

/*VARARGS2*/
char *
sprintf(string, format, args)
char *string, *format;
{
	FILE siop;

	siop._cnt = MAXINT;
	siop._base = siop._ptr = (unsigned char *)string;
	siop._flag = _IOWRT+_IOSTRG;
	(void) _doprnt(format, &args, &siop);
	*siop._ptr = '\0'; /* plant terminating null character */
	return(string);
}

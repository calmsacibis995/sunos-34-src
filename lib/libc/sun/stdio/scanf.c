#ifndef lint
static	char sccsid[] = "@(#)scanf.c 1.1 86/09/24 SMI"; /* from S5R2 1.3 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>

extern int _doscan();

/*VARARGS1*/
int
scanf(fmt, args)
char *fmt;
{
	return(_doscan(stdin, fmt, &args));
}

/*VARARGS2*/
int
fscanf(iop, fmt, args)
FILE *iop;
char *fmt;
{
	return(_doscan(iop, fmt, &args));
}

/*VARARGS2*/
int
sscanf(str, fmt, args)
register char *str;
char *fmt;
{
	FILE strbuf;

	strbuf._flag = _IOREAD|_IOSTRG;
	strbuf._ptr = strbuf._base = (unsigned char*)str;
	strbuf._cnt = strlen(str);
	strbuf._bufsiz = strbuf._cnt;
	return(_doscan(&strbuf, fmt, &args));
}

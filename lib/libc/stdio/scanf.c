#ifndef lint
static	char sccsid[] = "@(#)scanf.c 1.1 86/09/24 SMI"; /* from S5R2 1.3 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#include <varargs.h>

extern int _doscan();

/*VARARGS1*/
int
scanf(fmt, va_alist)
char *fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return(_doscan(stdin, fmt, ap));
}

/*VARARGS2*/
int
fscanf(iop, fmt, va_alist)
FILE *iop;
char *fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return(_doscan(iop, fmt, ap));
}

/*VARARGS2*/
int
sscanf(str, fmt, va_alist)
register char *str;
char *fmt;
va_dcl
{
	va_list ap;
	FILE strbuf;

	va_start(ap);
	strbuf._flag = _IOREAD|_IOSTRG;
	strbuf._ptr = strbuf._base = (unsigned char*)str;
	strbuf._cnt = strlen(str);
	strbuf._bufsiz = strbuf._cnt;
	return(_doscan(&strbuf, fmt, ap));
}

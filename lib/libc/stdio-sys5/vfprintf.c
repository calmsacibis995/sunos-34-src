#ifndef lint
static	char sccsid[] = "@(#)vfprintf.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#include <varargs.h>

extern int _doprnt();

/*VARARGS2*/
int
vfprintf(iop, format, ap)
FILE *iop;
char *format;
va_list ap;
{
	register int count;

	if (!(iop->_flag & _IOWRT)) {
		/* if no write flag */
		if (iop->_flag & _IORW) {
			/* if ok, cause read-write */
			iop->_flag |= _IOWRT;
		} else {
			/* else error */
			return EOF;
		}
	}
	count = _doprnt(format, ap, iop);
	return(ferror(iop)? EOF: count);
}

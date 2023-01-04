#ifndef lint
static	char sccsid[] = "@(#)fprintf.c 1.1 86/09/24 SMI"; /* from S5R2 1.5 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>

extern int _doprnt();

/*VARARGS2*/
int
fprintf(iop, format, args)
FILE *iop;
char *format;
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
	count = _doprnt(format, &args, iop);
	return(ferror(iop)? EOF: count);
}

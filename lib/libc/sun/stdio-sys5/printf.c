#ifndef lint
static	char sccsid[] = "@(#)printf.c 1.1 86/09/24 SMI"; /* from S5R2 1.5 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>

extern int _doprnt();

/*VARARGS1*/
int
printf(format, args)
char *format;
{
	register int count;

	if (!(stdout->_flag & _IOWRT)) {
		/* if no write flag */
		if (stdout->_flag & _IORW) {
			/* if ok, cause read-write */
			stdout->_flag |= _IOWRT;
		} else {
			/* else error */
			return EOF;
		}
	}
	count = _doprnt(format, &args, stdout);
	return(ferror(stdout)? EOF: count);
}

#ifndef lint
static	char sccsid[] = "@(#)scanw.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

/*
 * 1/26/81 (Berkeley) @(#)scanw.c	1.1
 */

# include	"curses.ext"
# include	<varargs.h>

/*
 *	This routine implements a scanf on the standard screen.
 */
/* VARARGS */
scanw(fmt, va_alist)
char	*fmt;
va_dcl
{
	va_list	ap;

	va_start(ap);
	return __sscans(stdscr, fmt, ap);
}

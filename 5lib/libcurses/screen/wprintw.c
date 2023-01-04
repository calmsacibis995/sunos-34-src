#ifndef lint
static	char sccsid[] = "@(#)wprintw.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"
# include	<varargs.h>

/*
 *	This routine implements a printf on the given window.
 */
/* VARARGS */
wprintw(win, fmt, va_alist)
WINDOW	*win;
char	*fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return _sprintw(win, fmt, ap);
}

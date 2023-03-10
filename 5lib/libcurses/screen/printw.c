#ifndef lint
static	char sccsid[] = "@(#)printw.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

/*
 * printw and friends
 *
 * 1/26/81 (Berkeley) @(#)printw.c	1.1
 */

# include	"curses.ext"
# include	<varargs.h>

/*
 *	This routine implements a printf on the standard screen.
 */
/* VARARGS */
printw(fmt, va_alist)
char	*fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return _sprintw(stdscr, fmt, ap);
}

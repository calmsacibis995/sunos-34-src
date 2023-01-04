#ifndef lint
static	char sccsid[] = "@(#)clear.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"

/*
 *	This routine clears the _window.
 *
 * 1/26/81 (Berkeley) @(#)clear.c	1.1
 */
wclear(win)
reg WINDOW	*win; {

	if (win == curscr)
		win = stdscr;
	werase(win);
	win->_clear = TRUE;
	return OK;
}

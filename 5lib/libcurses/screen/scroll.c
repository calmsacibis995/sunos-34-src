#ifndef lint
static	char sccsid[] = "@(#)scroll.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"

/*
 *	This routine scrolls the window up a line.
 *
 * 7/8/81 (Berkeley) @(#)scroll.c	1.2
 */
scroll(win)
WINDOW *win;
{
	_tscroll(win, 1);
}

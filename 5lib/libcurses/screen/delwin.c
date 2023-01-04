#ifndef lint
static	char sccsid[] = "@(#)delwin.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"

/*
 *	This routine deletes a _window and releases it back to the system.
 *
 * 1/26/81 (Berkeley) @(#)delwin.c	1.1
 */
delwin(win)
reg WINDOW	*win; {

	reg int	i;

	if (!(win->_flags & _SUBWIN))
		for (i = 0; i < win->_maxy && win->_y[i]; i++)
			cfree((char *) win->_y[i]);
	cfree((char *) win->_y);
	cfree((char *) win);
}

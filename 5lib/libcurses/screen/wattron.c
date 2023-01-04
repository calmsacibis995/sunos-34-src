#ifndef lint
static	char sccsid[] = "@(#)wattron.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

/*
 * 1/26/81 (Berkeley) @(#)standout.c	1.1
 */

# include	"curses.ext"

/*
 * Turn on selected attributes.
 */
wattron(win, attrs)
register WINDOW	*win;
int attrs;
{
#ifdef DEBUG
	if(outf) fprintf(outf, "WATTRON(%x, %o)\n", win, attrs);
#endif

	win->_attrs |= attrs;
	return 1;
}

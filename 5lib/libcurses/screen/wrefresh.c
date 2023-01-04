#ifndef lint
static	char sccsid[] = "@(#)wrefresh.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

/*
 * make the current screen look like "win" over the area covered by
 * win.
 *
 * 7/9/81 (Berkeley) @(#)refresh.c	1.6
 */

# include	"curses.ext"

/* Put out window and update screen */
wrefresh(win)
WINDOW	*win;
{
	extern int _endwin;

	/*
	 * If "win" is "curscr", the call to "wnoutrefresh" will just result
	 * in a call to "_ll_refresh"; the call to "doupdate" will then
	 * result in another one.  The first call will say "don't use
	 * insert/delete line", the second call will use it if "idlok"
	 * has been called on "curscr".  If "_endwin" is true. the screen
	 * will get cleared before the second call anyway, so the value of
	 * the "insert/delete OK" flag is irrelevant; don't paint the
	 * screen twice.
	 */
	if (win != curscr || !_endwin)
		wnoutrefresh(win);
	return doupdate();
}

#ifndef lint
static	char sccsid[] = "@(#)prefresh.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

/*
 * make the current screen look like "win" over the area covered by
 * win.
 *
 * 7/9/81 (Berkeley) @(#)refresh.c	1.6
 */

#include	"curses.ext"

/* Like wrefresh but refreshing from a pad. */
prefresh(pad, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol)
WINDOW	*pad;
int pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol;
{
	pnoutrefresh(pad, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol);
	return doupdate();
}

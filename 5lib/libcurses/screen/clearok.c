#ifndef lint
static	char sccsid[] = "@(#)clearok.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

clearok(win,bf)	
WINDOW *win;
int bf;
{
#ifdef DEBUG
	if (win == stdscr)
		fprintf("it's stdscr: ");
	if (win == curscr)
		fprintf("it's curscr: ");
	if (outf) fprintf(outf, "clearok(%x, %d)\n", win, bf);
#endif
	if (win==curscr)
		SP->doclear = 1;
	else
		win->_clear = bf;
}

#ifndef lint
static	char sccsid[] = "@(#)nodelay.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

/*
 * TRUE => don't wait for input, but return -1 instead.
 */
nodelay(win,bf)
WINDOW *win; int bf;
{
	_fixdelay(win->_nodelay, bf);
	win->_nodelay = bf;
}

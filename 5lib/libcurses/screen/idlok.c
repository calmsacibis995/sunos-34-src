#ifndef lint
static	char sccsid[] = "@(#)idlok.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

/*
 * TRUE => OK to use insert/delete line.
 */
idlok(win,bf)
WINDOW *win;
int bf;
{
	win->_use_idl = bf;
}

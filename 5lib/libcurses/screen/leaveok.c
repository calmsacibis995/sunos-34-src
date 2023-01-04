#ifndef lint
static	char sccsid[] = "@(#)leaveok.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

/*
 * TRUE => OK to leave cursor where it happens to fall after refresh.
 */
leaveok(win,bf)
WINDOW *win; int bf;
{
	win->_leave = bf;
}

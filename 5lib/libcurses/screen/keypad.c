#ifndef lint
static	char sccsid[] = "@(#)keypad.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

/*
 * TRUE => special keys should be passed as a single character by getch.
 */
keypad(win,bf)
WINDOW *win; int bf;
{
	win->_use_keypad = bf;
}

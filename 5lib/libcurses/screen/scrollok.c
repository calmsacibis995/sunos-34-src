#ifndef lint
static	char sccsid[] = "@(#)scrollok.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

/*
 * TRUE => OK to scroll screen up when you run off the bottom.
 */
scrollok(win,bf)
WINDOW *win;
int bf;
{
	/* Should consider using scroll/page mode of some terminals. */
	win->_scroll = bf;
}

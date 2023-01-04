#ifndef lint
static	char sccsid[] = "@(#)delch.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"

/*
 *	This routine performs an delete-char on the line, leaving
 * (_cury,_curx) unchanged.
 *
 * @(#)delch.c	1.1 (Berkeley) 4/17/81
 */
wdelch(win)
reg WINDOW	*win; {

	reg chtype	*temp1, *temp2;
	reg chtype	*end;

	end = &win->_y[win->_cury][win->_maxx - 1];
	temp2 = &win->_y[win->_cury][win->_curx + 1];
	temp1 = temp2 - 1;
	while (temp1 < end)
		*temp1++ = *temp2++;
	*temp1 = ' ';
	win->_lastch[win->_cury] = win->_maxx - 1;
	if (win->_firstch[win->_cury] == _NOCHANGE ||
	    win->_firstch[win->_cury] > win->_curx)
		win->_firstch[win->_cury] = win->_curx;
	return OK;
}

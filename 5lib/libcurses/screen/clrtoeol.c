#ifndef lint
static	char sccsid[] = "@(#)clrtoeol.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"

/*
 *	This routine clears up to the end of line
 *
 * 3/5/81 (Berkeley) @(#)clrtoeol.c	1.2
 */
wclrtoeol(win)
reg WINDOW	*win; {

	reg chtype	*sp, *end;
	reg int		y, x;
	reg chtype	*maxx;
	reg int		minx;

	y = win->_cury;
	x = win->_curx;
	end = &win->_y[y][win->_maxx];
	minx = _NOCHANGE;
	maxx = &win->_y[y][x];
	for (sp = maxx; sp < end; sp++)
		if (*sp != ' ') {
			maxx = sp;
			if (minx == _NOCHANGE)
				minx = sp - win->_y[y];
			*sp = ' ';
		}
	/*
	 * update firstch and lastch for the line
	 */
# ifdef DEBUG
	if(outf) fprintf(outf, "CLRTOEOL: line %d minx = %d, maxx = %d, firstch = %d, lastch = %d, next firstch %d\n", y, minx, maxx - win->_y[y], win->_firstch[y], win->_lastch[y], win->_firstch[y+1]);
# endif
	if (minx != _NOCHANGE) {
		if (win->_firstch[y] > minx || win->_firstch[y] == _NOCHANGE)
			win->_firstch[y] = minx;
		if (win->_lastch[y] < maxx - win->_y[y])
			win->_lastch[y] = maxx - win->_y[y];
	}
}

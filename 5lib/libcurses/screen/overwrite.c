#ifndef lint
static	char sccsid[] = "@(#)overwrite.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"

# define	min(a,b)	(a < b ? a : b)

/*
 *	This routine writes win1 on win2 destructively.
 *
 * 1/26/81 (Berkeley) @(#)overwrite.c	1.1
 */
overwrite(win1, win2)
reg WINDOW	*win1, *win2; {

	reg int		x, y, minx, miny, starty;

# ifdef DEBUG
	if(outf) fprintf(outf, "OVERWRITE(0%o, 0%o);\n", win1, win2);
# endif
	miny = min(win1->_maxy, win2->_maxy);
	minx = min(win1->_maxx, win2->_maxx);
# ifdef DEBUG
	if(outf) fprintf(outf, "OVERWRITE:\tminx = %d,  miny = %d\n", minx, miny);
# endif
	starty = win1->_begy - win2->_begy;
	for (y = 0; y < miny; y++)
		if (wmove(win2, y + starty, 0) != ERR)
			for (x = 0; x < minx; x++)
				waddch(win2, win1->_y[y][x]);
}

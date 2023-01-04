#ifndef lint
static	char sccsid[] = "@(#)mvwprintw.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"

/* VARARGS */
mvwprintw(win, y, x, fmt, args)
reg WINDOW	*win;
reg int		y, x;
char		*fmt;
int		args;
{

	return wmove(win, y, x) == OK ? _sprintw(win, fmt, &args) : ERR;
}

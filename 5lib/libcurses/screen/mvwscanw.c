#ifndef lint
static	char sccsid[] = "@(#)mvwscanw.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"

/* VARARGS */
mvwscanw(win, y, x, fmt, args)
reg WINDOW	*win;
reg int		y, x;
char		*fmt;
int		args; {

	return wmove(win, y, x) == OK ? __sscans(win, fmt, &args) : ERR;
}

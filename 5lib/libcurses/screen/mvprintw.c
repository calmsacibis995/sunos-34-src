#ifndef lint
static	char sccsid[] = "@(#)mvprintw.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"

/*
 * implement the mvprintw commands.  Due to the variable number of
 * arguments, they cannot be macros.  Sigh....
 *
 * 1/26/81 (Berkeley) @(#)mvprintw.c	1.1
 */

/* VARARGS */
mvprintw(y, x, fmt, args)
reg int		y, x;
char		*fmt;
int		args; {

	return move(y, x) == OK ? _sprintw(stdscr, fmt, &args) : ERR;
}

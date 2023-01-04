#ifndef lint
static	char sccsid[] = "@(#)mvscanw.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"

/*
 * implement the mvscanw commands.  Due to the variable number of
 * arguments, they cannot be macros.  Another sigh....
 *
 * 1/26/81 (Berkeley) @(#)mvscanw.c	1.1
 */

/* VARARGS */
mvscanw(y, x, fmt, args)
reg int		y, x;
char		*fmt;
int		args; {

	return move(y, x) == OK ? __sscans(stdscr, fmt, &args) : ERR;
}

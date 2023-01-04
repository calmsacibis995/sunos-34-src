/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)printw.c 1.1 86/09/25 SMI"; /* from UCB 5.1 85/06/07 */
#endif not lint

/*
 * printw and friends
 *
 */

# include	"curses.ext"

/*
 *	This routine implements a printf on the standard screen.
 */
printw(fmt, args)
char	*fmt;
int	args; {

	return _sprintw(stdscr, fmt, &args);
}

/*
 *	This routine implements a printf on the given window.
 */
wprintw(win, fmt, args)
WINDOW	*win;
char	*fmt;
int	args; {

	return _sprintw(win, fmt, &args);
}
/*
 *	This routine actually executes the printf and adds it to the window
 *
 *	This code now uses the vsprintf routine, which portably digs
 *	into stdio.  We provide a vsprintf for older systems that don't
 *	have one.
 */
_sprintw(win, fmt, args)
WINDOW	*win;
char	*fmt;
int	*args; {

	char	buf[512];

	vsprintf(buf, fmt, args);
	return waddstr(win, buf);
}

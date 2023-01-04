#ifndef lint
static	char sccsid[] = "@(#)m_clear.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"
# include	<signal.h>

/*
 *	mini.c contains versions of curses routines for minicurses.
 *	They work just like their non-mini counterparts but draw on
 *	std_body rather than stdscr.  This cuts down on overhead but
 *	restricts what you are allowed to do - you can't get stuff back
 *	from the screen and you can't use multiple windows or things
 *	like insert/delete line (the logical ones that affect the screen).
 *	All this but multiple windows could probably be added if somebody
 *	wanted to mess with it.
 *
 * 3/5/81 (Berkeley) @(#)addch.c	1.3
 */

/*
 * Like m_erase but ensures that the screen will be cleared on next refresh.
 */
m_clear()
{
#ifdef DEBUG
	if (outf) fprintf(outf, "M_CLEAR\n");
#endif
	SP->doclear = 1;
	m_erase();
}

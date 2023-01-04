#ifndef lint
static	char sccsid[] = "@(#)set_term.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

struct screen *
set_term(new)
struct screen *new;
{
	register struct screen *rv = SP;

#ifdef DEBUG
	if(outf) fprintf(outf, "setterm: old %x, new %x\n", rv, new);
#endif

#ifndef		NONSTANDARD
	SP = new;
#endif		NONSTANDARD

	cur_term = SP->tcap;
	LINES = lines;
	COLS = columns;
	stdscr = SP->std_scr;
	curscr = SP->cur_scr;
	return rv;
}

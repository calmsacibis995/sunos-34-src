#ifndef lint
static	char sccsid[] = "@(#)def_prog.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include	"curses.h"
#include	"term.h"

extern	struct term *cur_term;

def_prog_mode()
{
#ifdef USG
	ioctl(cur_term -> Filedes, TCGETA, &(cur_term->Nttyb));
#else
	ioctl(cur_term -> Filedes, TIOCGETP, &(cur_term->Nttyb));
#endif
}

#ifndef lint
static	char sccsid[] = "@(#)fixterm.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"
#include "../local/uparm.h"

extern	struct term *cur_term;

fixterm()
{
	reset_prog_mode();
}

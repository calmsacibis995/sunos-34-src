#ifndef lint
static	char sccsid[] = "@(#)resetterm.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"
#include "../local/uparm.h"

extern	struct term *cur_term;

resetterm()
{
	reset_shell_mode();
}

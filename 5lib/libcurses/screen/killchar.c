#ifndef lint
static	char sccsid[] = "@(#)killchar.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

char
killchar()
{
#ifdef USG
	return cur_term->Ottyb.c_cc[VKILL];
#else
	return cur_term->Ottyb.sg_kill;
#endif
}

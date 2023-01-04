#ifndef lint
static	char sccsid[] = "@(#)erasechar.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

char
erasechar()
{
#ifdef USG
	return cur_term->Ottyb.c_cc[VERASE];
#else
	return cur_term->Ottyb.sg_erase;
#endif
}

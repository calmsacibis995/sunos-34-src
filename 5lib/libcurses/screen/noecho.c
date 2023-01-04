#ifndef lint
static	char sccsid[] = "@(#)noecho.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

noecho()
{
	SP->fl_echoit = FALSE;
}

#ifndef lint
static	char sccsid[] = "@(#)echo.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

echo()	
{
	SP->fl_echoit = TRUE;
}

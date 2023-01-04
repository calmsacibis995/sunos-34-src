#ifndef lint
static	char sccsid[] = "@(#)baudrate.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

int
baudrate()
{
	return SP->baud;
}

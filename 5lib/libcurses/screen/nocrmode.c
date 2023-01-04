#ifndef lint
static	char sccsid[] = "@(#)nocrmode.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

nocrmode()
{
	nocbreak();
}

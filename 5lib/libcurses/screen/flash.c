#ifndef lint
static	char sccsid[] = "@(#)flash.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

extern	int	_outch();

flash()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "flash().\n");
#endif
    if (flash_screen)
	tputs (flash_screen, 0, _outch);
    else
	tputs (bell, 0, _outch);
    __cflush();
}

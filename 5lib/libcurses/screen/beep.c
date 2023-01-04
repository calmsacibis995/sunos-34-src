#ifndef lint
static	char sccsid[] = "@(#)beep.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

extern	int	_outch();

beep()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "beep().\n");
#endif
    if (bell)
	tputs (bell, 0, _outch);
    else
	tputs (flash_screen, 0, _outch);
    __cflush();
}

#ifndef lint
static	char sccsid[] = "@(#)clrerr.c 1.1 86/09/24 SMI"; /* from S5R2 1.3 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#undef clearerr

void
clearerr(iop)
register FILE *iop;
{
	iop->_flag &= ~(_IOERR | _IOEOF);
}

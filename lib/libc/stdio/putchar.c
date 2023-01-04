#ifndef lint
static	char sccsid[] = "@(#)putchar.c 1.1 86/09/24 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
/*
 * A subroutine version of the macro putchar
 */
#include <stdio.h>
#undef putchar

int
putchar(c)
register char c;
{
	return(putc(c, stdout));
}

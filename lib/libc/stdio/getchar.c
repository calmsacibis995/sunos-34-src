#ifndef lint
static	char sccsid[] = "@(#)getchar.c 1.1 86/09/24 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
/*
 * A subroutine version of the macro getchar.
 */
#include <stdio.h>
#undef getchar

int
getchar()
{
	return(getc(stdin));
}

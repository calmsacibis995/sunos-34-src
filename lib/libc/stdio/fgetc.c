#ifndef lint
static	char sccsid[] = "@(#)fgetc.c 1.1 86/09/24 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>

int
fgetc(fp)
register FILE *fp;
{
	return(getc(fp));
}

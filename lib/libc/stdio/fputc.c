#ifndef lint
static	char sccsid[] = "@(#)fputc.c 1.1 86/09/24 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>

int
fputc(c, fp)
int	c;
register FILE *fp;
{
	return(putc(c, fp));
}

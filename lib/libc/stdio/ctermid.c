#ifndef lint
static	char sccsid[] = "@(#)ctermid.c 1.1 86/09/24 SMI"; /* from S5R2 1.3 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>

extern char *strcpy();
static char res[L_ctermid];

char *
ctermid(s)
register char *s;
{
	return (strcpy(s != NULL ? s : res, "/dev/tty"));
}

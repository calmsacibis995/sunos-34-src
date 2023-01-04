#ifndef lint
static	char sccsid[] = "@(#)perror.c 1.1 86/09/24 SMI";
#endif

/*
 * Print the error indicated
 * in the cerror cell.
 */
#include <stdio.h>

extern	int fflush();
extern	void _perror();

void
perror(s)
	char *s;
{

	(void)fflush(stderr);
	_perror(s);
}

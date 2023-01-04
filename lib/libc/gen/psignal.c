#ifndef lint
static	char sccsid[] = "@(#)psignal.c 1.1 86/09/24 (C) 1983 SMI";
#endif

/*
 * Print the name of the signal indicated
 * along with the supplied message.
 */
#include <stdio.h>

extern	int fflush();
extern	void _psignal();

void
psignal(sig, s)
	unsigned sig;
	char *s;
{

	(void)fflush(stderr);
	_psignal(sig, s);
}

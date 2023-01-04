#ifndef lint
static	char sccsid[] = "@(#)subr.c 1.4 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - miscellaneous subroutines
 */

#include <stdio.h>
#include <ctype.h>

char	*strcpy();

/*
 * Save a copy of a string.
 */
char *
mt_savestr(s)
	register char *s;
{
	register char *t;
	extern char *malloc();
	extern char *mt_cmdname;

	t = malloc((unsigned)(strlen(s) + 1));
	if (t == NULL) {
		(void)fprintf(stderr, "%s: Out of memory!\n", mt_cmdname);
		mt_stop_mail(0);
		mt_done(1);
	}
	(void)strcpy(t, s);
	return (t);
}

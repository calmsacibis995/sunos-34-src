#ifndef lint
static char sccsid[] = "@(#)screenutil.c 1.2 87/01/08 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <varargs.h>

char *Progname = "";

char *
basename(path)
	char *path;
{
	register char *p = path, c;

	while (c = *p++)
		if (c == '/')
			path = p;

	return path;
}

/*VARARGS*/
void
error(va_alist)
va_dcl
{
	va_list ap;
	char *fmt;

	va_start(ap);

	if (fmt = va_arg(ap, char *)) 
		(void) fprintf(stderr, "%s: ", Progname);
	else
		fmt = va_arg(ap, char *);

	(void) _doprnt(fmt, ap, stderr);
	va_end(ap);

	(void) fprintf(stderr, "\n");

	exit(1);
}

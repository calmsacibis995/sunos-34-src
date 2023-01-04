#ifndef lint
static	char sccsid[] = "@(#)error.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <stdio.h>

/*
 * error - 
 */
error(fmt, a2, a3, a4, a5, a6, a7, a8, a9)
char 	*fmt;
{
	fprintf(stderr, "%s: %d: ", filename(), lineno());
	fprintf(stderr, fmt, a2, a3, a4, a5, a6, a7, a8, a9);
	fprintf(stderr, "\n");
	exit(1);
}


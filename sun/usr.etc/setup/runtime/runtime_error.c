
#ifndef lint
static  char sccsid[] = "@(#)runtime_error.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include	<stdio.h>

/*VARARGS*/
runtime_error(fmt, arg2, arg3, arg4, arg5, arg6, arg7)
char	*fmt;
{
	fprintf(stderr, "setup runtime error: ");
	fprintf(stderr, fmt, arg2, arg3, arg4, arg5, arg6, arg7);
	fprintf(stderr, "\n");
	abort();
}

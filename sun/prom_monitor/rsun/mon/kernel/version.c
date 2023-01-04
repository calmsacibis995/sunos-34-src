#ifndef lint
static	char sccsid[] =
	"@(#)version.c 2.8 85/02/19 Copyright (c) 1985 by Sun Microsystems, Inc.";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Fake out "make depend" into believing that we depend on the Makefile.
 * (we do, for "ID")...
 */
#ifdef MAKE_DEPEND
#include "Makefile"
#endif

char monrev[] = ID;

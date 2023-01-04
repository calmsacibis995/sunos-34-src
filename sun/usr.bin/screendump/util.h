/*      @(#)util.h 1.1 86/09/25 SMI      */

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Common data definitions and constant definitions for
 *	screenload, screendump, and rastrepl.
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <pixrect/pixrect_hs.h>

#define FALSE		0
#define TRUE		!FALSE

#define	SERROR(err_message)						\
	{fprintf(stderr, "%s: %s\n", Progname, err_message); exit(1);}
	/*
	 * This macro assumes that the module containing main initializes
	 *   the global string Progname, and that Progname is known in the
	 *   scope of the macro call.
	 */


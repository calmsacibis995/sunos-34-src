#ifndef lint
static	char sccsid[] = "@(#)conf.c	1.1 86/09/25	Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Configuration table for standalone I/O system.
 *
 * This table lists all the supported drivers.  It is searched by open()
 * to parse the device specification.
 */

#include "saio.h"

extern struct boottab xydriver;
extern struct boottab sddriver;
extern struct boottab stdriver;
extern struct boottab mtdriver;
extern struct boottab xtdriver;
extern struct boottab iedriver;
#ifdef SUN3
extern struct boottab ledriver;
#endif
#ifdef SUN2
extern struct boottab ipdriver;
extern struct boottab ecdriver;
extern struct boottab ardriver;
#endif

/*
 * The device table 
 */
struct boottab *(devsw[]) = {
	&xydriver,
	&sddriver,
	&stdriver,
	&mtdriver,
	&xtdriver,
	&iedriver,
#ifdef SUN3
	&ledriver,
#endif
#ifdef SUN2
	&ipdriver,
	&ecdriver,
	&ardriver,
#endif
	(struct boottab *)0,
};

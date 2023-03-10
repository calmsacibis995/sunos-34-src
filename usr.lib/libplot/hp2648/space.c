/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)space.c 1.1 86/09/25 SMI"; /* from UCB 5.1 5/7/85 */
#endif not lint

#include "hp2648.h"

space(x0,y0,x1,y1)
int x0,y0,x1,y1;
{
	lowx = x0;
	lowy = y0;
	scalex = 720.0/(x1-lowx);
	scaley = 360.0/(y1-lowy);
}

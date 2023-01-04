#ifndef lint
static char sccsid[] = "@(#)polygon277.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int polygon_abs_2();
int polygon_rel_2();

int polygonabs2_(xlist, ylist, n)
float *xlist, *ylist;
int *n;
	{
	return(polygon_abs_2(xlist, ylist, *n));
	}

int polygonrel2_(xlist, ylist, n)
float *xlist, *ylist;
int *n;
	{
	return(polygon_rel_2(xlist, ylist, *n));
	}

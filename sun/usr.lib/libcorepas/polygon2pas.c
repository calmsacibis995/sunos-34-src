#ifndef lint
static char sccsid[] = "@(#)polygon2pas.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

#include "pasarray.h"

int polygon_abs_2();
int polygon_rel_2();

int polygonabs2(xlist, ylist, n)
double xlist[], ylist[];
int n;
	{
		int i;
		for (i = 0; i < n; ++i) {
			xcoort[i] = xlist[i];
			ycoort[i] = ylist[i];
		}
		return(polygon_abs_2(xcoort, ycoort, n));
	}

int polygonrel2(xlist, ylist, n)
double xlist[], ylist[];
int n;
	{
		int i;
		for (i = 0; i < n; ++i) {
			xcoort[i] = xlist[i];
			ycoort[i] = ylist[i];
		}
		return(polygon_rel_2(xcoort, ycoort, n));
	}

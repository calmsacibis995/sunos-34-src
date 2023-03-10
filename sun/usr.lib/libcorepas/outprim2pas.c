#ifndef lint
static char sccsid[] = "@(#)outprim2pas.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

#include "pasarray.h"

int inquire_current_position_2();
int line_abs_2();
int line_rel_2();
int move_abs_2();
int move_rel_2();
polyline_abs_2();
polyline_rel_2();

int inqcurrpos2(x, y)
double *x, *y;
	{
	float tx, ty;
	int f;
	f=inquire_current_position_3(&tx, &ty);
	*x=tx;
	*y=ty;
	return(f);
	}

int lineabs2(x, y)
double x, y;
	{
	return(line_abs_2(x, y));
	}

int linerel2(dx, dy)
double dx, dy;
	{
	return(line_rel_2(dx, dy));
	}

int moveabs2(x, y)
double x, y;
	{
	return(move_abs_2(x, y));
	}

int moverel2(dx, dy)
double dx, dy;
	{
	return(move_rel_2(dx, dy));
	}

int polylineabs2(xcoord, ycoord, n)
double xcoord[], ycoord[];
int n;
	{
		int i;
		for (i = 0; i < n; ++i) {
			xcoort[i] = xcoord[i];
			ycoort[i] = ycoord[i];
		}
		return(polyline_abs_2(xcoort, ycoort, n));
	}

int polylinerel2(xcoord, ycoord, n)
double xcoord[], ycoord[];
int n;
	{
		int i;
		for (i = 0; i < n; ++i) {
			xcoort[i] = xcoord[i];
			ycoort[i] = ycoord[i];
		}
		return(polyline_rel_2(xcoort, ycoort, n));
	}

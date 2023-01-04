#ifndef lint
static char sccsid[] = "@(#)outprim277.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int inquire_current_position_2();
int line_abs_2();
int line_rel_2();
int move_abs_2();
int move_rel_2();
polyline_abs_2();
polyline_rel_2();

int inqcurrpos2_(x, y)
float *x, *y;
	{
	return(inquire_current_position_2(x, y));
	}

int lineabs2_(x, y)
float *x, *y;
	{
	return(line_abs_2(*x, *y));
	}

int linerel2_(dx, dy)
float *dx, *dy;
	{
	return(line_rel_2(*dx, *dy));
	}

int moveabs2_(x, y)
float *x, *y;
	{
	return(move_abs_2(*x, *y));
	}

int moverel2_(dx, dy)
float *dx, *dy;
	{
	return(move_rel_2(*dx, *dy));
	}

int polylineabs2_(xcoord, ycoord, n)
float xcoord[], ycoord[];
int *n;
	{
	return(polyline_abs_2(xcoord, ycoord, *n));
	}

int polylinerel2_(xcoord, ycoord, n)
float xcoord[], ycoord[];
int *n;
	{
	return(polyline_rel_2(xcoord, ycoord, *n));
	}

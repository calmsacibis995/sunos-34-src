#ifndef lint
static char sccsid[] = "@(#)outprim377.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int inquire_current_position_3();
int line_abs_3();
int line_rel_3();
int move_abs_3();
int move_rel_3();
int polyline_abs_3();
int polyline_rel_3();

int inqcurrpos3_(x, y, z)
float *x, *y, *z;
	{
	return(inquire_current_position_3(x, y, z));
	}

int lineabs3_(x, y, z)
float *x, *y, *z;
	{
	return(line_abs_3(*x, *y, *z));
	}

int linerel3_(dx, dy, dz)
float *dx, *dy, *dz;
	{
	return(line_rel_3(*dx, *dy, *dz));
	}

int moveabs3_(x, y, z)
float *x, *y, *z;
	{
	return(move_abs_3(*x, *y, *z));
	}

int moverel3_(dx, dy, dz)
float *dx, *dy, *dz;
	{
	return(move_rel_3(*dx, *dy, *dz));
	}

int polylineabs3_(xcoord, ycoord, zcoord, n)
float xcoord[], ycoord[], zcoord[];
int *n;
	{
	return(polyline_abs_3(xcoord, ycoord, zcoord, *n));
	}

int polylinerel3_(xcoord, ycoord, zcoord, n)
float xcoord[], ycoord[], zcoord[];
int *n;
	{
	return(polyline_rel_3(xcoord, ycoord, zcoord, *n));
	}

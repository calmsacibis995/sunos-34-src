#ifndef lint
static char sccsid[] = "@(#)textmark77.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "f77strings.h"

int marker_abs_2();
int marker_abs_3();
int marker_rel_2();
int marker_rel_3();
int polymarker_abs_2();
int polymarker_abs_3();
int polymarker_rel_2();
int polymarker_rel_3();
int text();

int markerabs2_(mx, my)
float *mx, *my;
	{
	return(marker_abs_2(*mx, *my));
	}

int markerabs3_(mx, my, mz)
float *mx, *my, *mz;
	{
	return(marker_abs_3(*mx, *my, *mz));
	}

int markerrel2_(dx, dy)
float *dx, *dy;
	{
	return(marker_rel_2(*dx, *dy));
	}

int markerrel3_(dx, dy, dz)
float *dx, *dy, *dz;
	{
	return(marker_rel_3(*dx, *dy, *dz));
	}

int polymarkerabs2_(xcoord, ycoord, n)
float xcoord[], ycoord[];
int *n;
	{
	return(polymarker_abs_2(xcoord, ycoord, *n));
	}

int polymarkerabs3_(xcoord, ycoord, zcoord, n)
float xcoord[], ycoord[], zcoord[];
int *n;
	{
	return(polymarker_abs_3(xcoord, ycoord, zcoord, *n));
	}

int polymarkerrel2_(xcoord, ycoord, n)
float xcoord[], ycoord[];
int *n;
	{
	return(polymarker_rel_2(xcoord, ycoord, *n));
	}

int polymarkerrel3_(xcoord, ycoord, zcoord, n)
float xcoord[], ycoord[], zcoord[];
int *n;
	{
	return(polymarker_rel_3(xcoord, ycoord, zcoord, *n));
	}

int text_(f77string, f77strleng)
char *f77string;
int f77strleng;
	{
	char *sptr;

	f77strleng = f77strleng > MAXLEN ? MAXLEN : f77strleng;
	sptr = _core_77string;
	while (f77strleng--)
		*sptr++ = *f77string++;
	*sptr = '\0';
	return(text(_core_77string));
	}

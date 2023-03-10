#ifndef lint
static char sccsid[] = "@(#)textmarkpas.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

#include "pasarray.h"

int marker_abs_2();
int marker_abs_3();
int marker_rel_2();
int marker_rel_3();
int polymarker_abs_2();
int polymarker_abs_3();
int polymarker_rel_2();
int polymarker_rel_3();
int text();

int markerabs2(mx, my)
double mx, my;
	{
	return(marker_abs_2(mx, my));
	}

int markerabs3(mx, my, mz)
double mx, my, mz;
	{
	return(marker_abs_3(mx, my, mz));
	}

int markerrel2(dx, dy)
double dx, dy;
	{
	return(marker_rel_2(dx, dy));
	}

int markerrel3(dx, dy, dz)
double dx, dy, dz;
	{
	return(marker_rel_3(dx, dy, dz));
	}

int polymarkerabs2(xcoord, ycoord, n)
double xcoord[], ycoord[];
int n;
	{
		int i;
		for (i = 0; i < n; ++i) {
			xcoort[i] = xcoord[i];
			ycoort[i] = ycoord[i];
		}
		return(polymarker_abs_2(xcoort, ycoort, n));
	}

int polymarkerabs3(xcoord, ycoord, zcoord, n)
double xcoord[], ycoord[], zcoord[];
int n;
	{
		int i;
		for (i = 0; i < n; ++i) {
			xcoort[i] = xcoord[i];
			ycoort[i] = ycoord[i];
			zcoort[i] = zcoord[i];
		}
		return(polymarker_abs_3(xcoort, ycoort, zcoort, n));
	}

int polymarkerrel2(xcoord, ycoord, n)
double xcoord[], ycoord[];
int n;
	{
		int i;
		for (i = 0; i < n; ++i) {
			xcoort[i] = xcoord[i];
			ycoort[i] = ycoord[i];
		}
		return(polymarker_rel_2(xcoort, ycoort, n));
	}

int polymarkerrel3(xcoord, ycoord, zcoord, n)
double xcoord[], ycoord[], zcoord[];
int n;
	{
		int i;
		for (i = 0; i < n; ++i) {
			xcoort[i] = xcoord[i];
			ycoort[i] = ycoord[i];
			zcoort[i] = zcoord[i];
		}
		return(polymarker_rel_3(xcoort, ycoort, zcoort, n));
	}

int puttext(f77string)
char *f77string;
	{
	char *sptr;
	char pasarg[257];
	int i,strlen;

	strlen = 256;
	sptr = f77string+256;
	while ((*--sptr) == ' ') {strlen--;};
	strncpy (pasarg,f77string,strlen);
	pasarg[strlen] = '\0';
	i = text(pasarg);
	return(i);
	}

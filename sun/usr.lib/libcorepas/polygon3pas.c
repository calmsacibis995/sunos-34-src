#ifndef lint
static char sccsid[] = "@(#)polygon3pas.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

#include "pasarray.h"

int polygon_abs_3();
int polygon_rel_3();
int set_light_direction();
int set_shading_parameters();
int set_vertex_indices();
int set_vertex_normals();

int polygonabs3(xlist, ylist, zlist, n)
double xlist[], ylist[], zlist[];
int n;
	{
		int i;
		for (i = 0; i < n; ++i) {
			xcoort[i] = xlist[i];
			ycoort[i] = ylist[i];
			zcoort[i] = zlist[i];
		}
		return(polygon_abs_3(xcoort, ycoort, zcoort, n));
	}

int polygonrel3(xlist, ylist, zlist, n)
double xlist[], ylist[], zlist[];
int n;
	{
		int i;
		for (i = 0; i < n; ++i) {
			xcoort[i] = xlist[i];
			ycoort[i] = ylist[i];
			zcoort[i] = zlist[i];
		}
		return(polygon_rel_3(xcoort, ycoort, zcoort, n));
	}

int setlightdirect(dx, dy, dz)
double dx, dy, dz;
	{
	return(set_light_direction(dx, dy, dz));
	}

int setshadingparams(amb, dif, spec, flood, bump, hue, style)
double amb, dif, spec, flood, bump;
int hue, style;
	{
	return(set_shading_parameters(amb, dif, spec, flood,
					bump, hue, style));
	}

int setvertexindices(indxlist, n)
int indxlist[], n;
	{
		int i;
		for (i = 0; i < n; ++i) {
			xint[i] = indxlist[i];
		}
		return(set_vertex_indices(xint, n));
	}

int setvertexnormals(dxlist, dylist, dzlist, n)
int n;
double dxlist[], dylist[], dzlist[];
	{
		int i;
		for (i = 0; i < n; ++i) {
			xcoort[i] = dxlist[i];
			ycoort[i] = dylist[i];
			zcoort[i] = dzlist[i];
		}
		return(set_vertex_normals(xcoort, ycoort, zcoort, n));
	}

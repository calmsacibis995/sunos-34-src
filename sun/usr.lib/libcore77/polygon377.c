#ifndef lint
static char sccsid[] = "@(#)polygon377.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

int set_zbuffer_cut();
int polygon_abs_3();
int polygon_rel_3();
int set_light_direction();
int set_shading_parameters();
int set_vertex_indices();
int set_vertex_normals();

#define DEVNAMESIZE 20

struct vwsurf	{
		char screenname[DEVNAMESIZE];
		char windowname[DEVNAMESIZE];
		int windowfd;
		int (*dd)();
		int instance;
		int cmapsize;
		char cmapname[DEVNAMESIZE];
		int flags;
		char **ptr;
		};

int setzbuffercut_(surfname, xlist, zlist, n)
float xlist[], zlist[];
int *n;
struct vwsurf *surfname;
	{
	return(set_zbuffer_cut(surfname, xlist, zlist, *n));
	}

int polygonabs3_(xlist, ylist, zlist, n)
float *xlist, *ylist, *zlist;
int *n;
	{
	return(polygon_abs_3(xlist, ylist, zlist, *n));
	}

int polygonrel3_(xlist, ylist, zlist, n)
float *xlist, *ylist, *zlist;
int *n;
	{
	return(polygon_rel_3(xlist, ylist, zlist, *n));
	}

int setlightdirect_(dx, dy, dz)
float *dx, *dy, *dz;
	{
	return(set_light_direction(*dx, *dy, *dz));
	}

int setshadingparams(amb, dif, spec, flood, bump, hue, style)
float *amb, *dif, *spec, *flood, *bump;
int *hue, *style;
	{
	return(set_shading_parameters(*amb, *dif, *spec, *flood,
					*bump, *hue, *style));
	}

int setvertexindices(indxlist, n)
int *indxlist, *n;
	{
	return(set_vertex_indices(indxlist, *n));
	}

int setvertexnormals(dxlist, dylist, dzlist, n)
int *n;
float *dxlist, *dylist, *dzlist;
	{
	return(set_vertex_normals(dxlist, dylist, dzlist, *n));
	}

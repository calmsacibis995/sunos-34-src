#ifndef lint
static char sccsid[] = "@(#)xformattrib77.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int set_ndc_space_2();
int set_ndc_space_3();
int set_projection();
int set_view_depth();
int set_view_plane_distance();
int set_view_plane_normal();
int set_view_reference_point();
int set_view_up_2();
int set_view_up_3();
int set_viewing_parameters();
int set_viewport_2();
int set_viewport_3();
int set_window();

int setndcspace2_(width, height)
float *width, *height;
	{
	return(set_ndc_space_2(*width, *height));
	}

int setndcspace3_(width, height, depth)
float *width, *height, *depth;
	{
	return(set_ndc_space_3(*width, *height, *depth));
	}

int setprojection_(projtype, dx, dy, dz)
int *projtype;
float *dx, *dy, *dz;
	{
	return(set_projection(*projtype, *dx, *dy, *dz));
	}

int setviewdepth_(near, far)
float *near, *far;
	{
	return(set_view_depth(*near, *far));
	}

int setviewplanedist(dist)
float *dist;
	{
	return(set_view_plane_distance(*dist));
	}

int setviewplanenorm(dx, dy, dz)
float *dx, *dy, *dz;
	{
	return(set_view_plane_normal(*dx, *dy, *dz));
	}

int setviewrefpoint_(x, y, z)
float *x, *y, *z;
	{
	return(set_view_reference_point(*x, *y, *z));
	}

int setviewup2_(dx, dy)
float *dx, *dy;
	{
	return(set_view_up_2(*dx, *dy));
	}

int setviewup3_(dx, dy, dz)
float *dx, *dy, *dz;
	{
	return(set_view_up_3(*dx, *dy, *dz));
	}

typedef struct {
	float xmin, xmax, ymin, ymax; } windtype;
typedef struct {
	float xmin,xmax,ymin,ymax,zmin,zmax; } porttype;

typedef struct {
      float vwrefpt[3];
      float vwplnorm[3];
      float viewdis;
      float frontdis;
      float backdis;
      int projtype;
      float projdir[3];
      windtype window;
      float vwupdir[3];
      porttype viewport;
      } vwprmtype;

int setviewingparams(viewparm)
vwprmtype *viewparm;
	{
	return(set_viewing_parameters(viewparm));
	}

int setviewport2_(xmin, xmax, ymin, ymax)
float *xmin, *xmax, *ymin, *ymax;
	{
	return(set_viewport_2(*xmin, *xmax, *ymin, *ymax));
	}

int setviewport3_(xmin, xmax, ymin, ymax, zmin, zmax)
float *xmin, *xmax, *ymin, *ymax, *zmin, *zmax;
	{
	return(set_viewport_3(*xmin, *xmax, *ymin, *ymax, *zmin, *zmax));
	}

int setwindow_(umin, umax, vmin, vmax)
float *umin, *umax, *vmin, *vmax;
	{
	return(set_window(*umin, *umax, *vmin, *vmax));
	}

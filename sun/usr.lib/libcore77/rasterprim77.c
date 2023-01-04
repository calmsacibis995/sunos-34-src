#ifndef lint
static char sccsid[] = "@(#)rasterprim77.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

int allocate_raster();
int free_raster();
int get_raster();
int put_raster();
int size_raster();

typedef struct {	/* RASTER, pixel coords */
	int width, height, depth;	/* width, height in pixels, bits/pixl */
	short *bits;
	} rast_type;

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

int allocateraster_(rptr)
rast_type *rptr;
	{
	return(allocate_raster(rptr));
	}

int freeraster_(rptr)
rast_type *rptr;
	{
	return(free_raster(rptr));
	}

int getraster_(surfname, xmin, xmax, ymin, ymax, xd, yd, raster)
struct vwsurf *surfname;
float *xmin, *xmax, *ymin, *ymax;
int *xd, *yd;
rast_type *raster;
	{
	return(get_raster(surfname, *xmin, *xmax, *ymin, *ymax,
					*xd, *yd, raster));
	}

int putraster_(srast)
rast_type *srast;
	{
	return(put_raster(srast));
	}

int sizeraster_(surfname, xmin, xmax, ymin, ymax, raster)
struct vwsurf *surfname;
float *xmin, *xmax, *ymin, *ymax;
rast_type *raster;
	{
	return(size_raster(surfname, *xmin, *xmax, *ymin, *ymax, raster));
	}

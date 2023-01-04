#ifndef lint
static char sccsid[] = "@(#)rasterprimpas.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
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

typedef struct  {
    		char screenname[DEVNAMESIZE];
    		char windowname[DEVNAMESIZE];
		int fd;
		int (*dd)();
		int instance;
		int cmapsize;
    		char cmapname[DEVNAMESIZE];
		int flags;
		char *ptr;
		} vwsurf;

int allocateraster(rptr)
rast_type *rptr;
	{
	return(allocate_raster(rptr));
	}

int freeraster(rptr)
rast_type *rptr;
	{
	return(free_raster(rptr));
	}

int getraster(surfname, xmin, xmax, ymin, ymax, xd, yd, raster)
vwsurf *surfname;
double xmin, xmax, ymin, ymax;
int xd, yd;
rast_type *raster;
	{
	return(get_raster(surfname, xmin, xmax, ymin, ymax,
					xd, yd, raster));
	}

int putraster(srast)
rast_type *srast;
	{
	return(put_raster(srast));
	}

int sizeraster(surfname, xmin, xmax, ymin, ymax, raster)
vwsurf *surfname;
double xmin, xmax, ymin, ymax;
rast_type *raster;
	{
	return(size_raster(surfname, xmin, xmax, ymin, ymax, raster));
	}

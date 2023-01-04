#ifndef lint
static char sccsid[] = "@(#)rasterfileio77.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

int file_to_raster();
int raster_to_file();

typedef struct {	/* RASTER, pixel coords */
	int width, height, depth;	/* width, height in pixels, bits/pixl */
	short *bits;
	} rast_type;

typedef struct {	/* colormap struct */
	int type, nbytes;
	char *data;
	} colormap_type;

int getfd_();

int filetoraster_(rasfid, raster, map)
int *rasfid;
rast_type *raster;
colormap_type *map;
	{
	return(file_to_raster(getfd_(rasfid), raster, map));
	}

#include "/usr/src/usr.lib/libI77/fio.h"

int rastertofile_(raster, map, rasfid, n)
rast_type *raster;
colormap_type *map;
int *rasfid, *n;
	{
	unit *u;

	u = mapunit(*rasfid);
	nowwriting(u);
	return(raster_to_file(raster, map, getfd_(rasfid), *n));
	}

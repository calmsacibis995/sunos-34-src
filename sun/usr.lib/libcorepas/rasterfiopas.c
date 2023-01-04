#ifndef lint
static char sccsid[] = "@(#)rasterfiopas.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/
#include <stdio.h>

int file_to_raster();
int raster_to_file();
FILE *ACTFILE();
char *UNIT();

typedef struct {	/* RASTER, pixel coords */
	int width, height, depth;	/* width, height in pixels, bits/pixl */
	short *bits;
	} rast_type;

typedef struct {	/* colormap struct */
	int type, nbytes;
	char *data;
	} colormap_type;


int filetoraster(rasfid, raster, map)
char *rasfid;
rast_type *raster;
colormap_type *map;
	{
	int fid;
	fid= fileno(ACTFILE(UNIT(rasfid)));
	return(file_to_raster(fid, raster, map));
	}

int rastertofile(raster, map, rasfid, n)
rast_type *raster;
colormap_type *map;
char *rasfid;
int n;
	{
	int fid;
	fid= fileno(ACTFILE(UNIT(rasfid)));
	return(raster_to_file(raster, map, rasfid, n));
	}


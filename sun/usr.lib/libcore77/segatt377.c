#ifndef lint
static char sccsid[] = "@(#)segatt377.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int inquire_segment_image_transformation_3();
int inquire_segment_image_translate_3();
int set_segment_image_transformation_3();
int set_segment_image_translate_3();

int inqsegimgxform3_(segname, sx, sy, sz, rx, ry, rz, tx, ty, tz)
int *segname;
float *sx, *sy, *sz, *rx, *ry, *rz, *tx, *ty, *tz;
	{
	return(inquire_segment_image_transformation_3(*segname, sx, sy, sz,
						      rx, ry, rz, tx, ty, tz));
	}

int inqsegimgxlate3_(segname, tx, ty, tz)
int *segname;
float *tx, *ty, *tz;
	{
	return(inquire_segment_image_translate_3(*segname, tx, ty, tz));
	}

int setsegimgxform3_(segname, sx, sy, sz, rx, ry, rz, tx, ty, tz)
int *segname;
float *sx, *sy, *sz, *rx, *ry, *rz, *tx, *ty, *tz;
	{
	return(set_segment_image_transformation_3(*segname, *sx, *sy, *sz,
						 *rx, *ry, *rz, *tx, *ty, *tz));
	}

int setsegimgxlate3_(segname, dx, dy, dz)
int *segname;
float *dx, *dy, *dz;
	{
	return(set_segment_image_translate_3(*segname, *dx, *dy, *dz));
	}

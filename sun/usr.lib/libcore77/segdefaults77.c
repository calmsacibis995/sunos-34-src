#ifndef lint
static char sccsid[] = "@(#)segdefaults77.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int inquire_detectability();
int inquire_highlighting();
int inquire_image_transformation_2();
int inquire_image_transformation_3();
int inquire_image_translate_2();
int inquire_image_translate_3();
int inquire_visibility();
int set_detectability();
int set_highlighting();
int set_image_transformation_2();
int set_image_transformation_3();
int set_image_translate_2();
int set_image_translate_3();
int set_visibility();

int inqdetectability(detectability)
int *detectability;
	{
	return(inquire_detectability(detectability));
	}

int inqhighlighting_(highlighting)
int *highlighting;
	{
	return(inquire_highlighting(highlighting));
	}

int inqimgtransform2(sx, sy, a, tx, ty)
float *sx, *sy, *a, *tx, *ty;
	{
	return(inquire_image_transformation_2(sx, sy, a, tx, ty));
	}

int inqimgtransform3(sx, sy, sz, ax, ay, az, tx, ty, tz)
float *sx, *sy, *sz, *ax, *ay, *az, *tx, *ty, *tz;
	{
	return(inquire_image_transformation_3(sx, sy, sz, ax, ay, az,
						tx, ty, tz));
	}

int inqimgtranslate2(tx, ty)
float *tx, *ty;
	{
	return(inquire_image_translate_2(tx, ty));
	}

int inqimgtranslate3(tx, ty, tz)
float *tx, *ty, *tz;
	{
	return(inquire_image_translate_3(tx, ty, tz));
	}

int inqvisibility_(visibility)
int *visibility;
	{
	return(inquire_visibility(visibility));
	}

int setdetectability(detectability)
int *detectability;
	{
	return(set_detectability(*detectability));
	}

int sethighlighting_(highlighting)
int *highlighting;
	{
	return(set_highlighting(*highlighting));
	}

int setimgtransform2(sx, sy, a, tx, ty)
float *sx, *sy, *a, *tx, *ty;
	{
	return(set_image_transformation_2(*sx, *sy, *a, *tx, *ty));
	}

int setimgtransform3(sx, sy, sz, ax, ay, az, tx, ty, tz)
float *sx, *sy, *sz, *ax, *ay, *az, *tx, *ty, *tz;
	{
	return(set_image_transformation_3(*sx, *sy, *sz, *ax, *ay,
						*az, *tx, *ty, *tz));
	}

int setimgtranslate2(tx, ty)
float *tx, *ty;
	{
	return(set_image_translate_2(*tx, *ty));
	}

int setimgtranslate3(tx, ty, tz)
float *tx, *ty, *tz;
	{
	return(set_image_translate_3(*tx, *ty, *tz));
	}

int setvisibility_(visibility)
int *visibility;
	{
	return(set_visibility(*visibility));
	}

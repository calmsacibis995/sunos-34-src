#ifndef lint
static char sccsid[] = "@(#)inqsegatt77.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int inquire_image_transformation_type();
int inquire_segment_detectability();
int inquire_segment_highlighting();
int inquire_segment_image_transformation_2();
int inquire_segment_image_transformation_type();
int inquire_segment_image_translate_2();
int inquire_segment_visibility();

int inqimgxformtype_(segtype)
int *segtype;
	{
	return(inquire_image_transformation_type(segtype));
	}

int inqsegdetectable(segname, detectbl)
int *segname, *detectbl;
	{
	return(inquire_segment_detectability(*segname, detectbl));
	}

int inqseghighlight_(segname, highlght)
int *segname, *highlght;
	{
	return(inquire_segment_highlighting(*segname, highlght));
	}

int inqsegimgxform2_(segname, sx, sy, a, tx, ty)
int *segname;
float *sx, *sy, *a, *tx, *ty;
	{
	return(inquire_segment_image_transformation_2(*segname,sx,sy,a,tx,ty));
	}

int inqsegimgxfrmtyp(segname, segtype)
int *segname, *segtype;
	{
	return(inquire_segment_image_transformation_type(*segname, segtype));
	}

int inqsegimgxlate2_(segname, tx, ty)
int *segname;
float *tx, *ty;
	{
	return(inquire_segment_image_translate_2(*segname, tx, ty));
	}

int inqsegvisibility(segname, visbilty)
int *segname, *visbilty;
	{
	return(inquire_segment_visibility(*segname, visbilty));
	}

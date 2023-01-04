#ifndef lint
static char sccsid[] = "@(#)credelsegpas.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

int close_retained_segment();
int close_temporary_segment();
int create_retained_segment();
int create_temporary_segment();
int set_image_transformation_type();

int closeretainseg(dummy)
int dummy;
	{
	return(close_retained_segment());
	}

int closetempseg(dummy)
int dummy;
	{
	return(close_temporary_segment());
	}

int createretainseg(segname)
int segname;
	{
	return(create_retained_segment(segname));
	}

int createtempseg(dummy)
int dummy;
	{
	return(create_temporary_segment());
	}

int setimgxformtype(segtype)
int segtype;
	{
	return(set_image_transformation_type(segtype));
	}

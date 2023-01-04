#ifndef lint
static char sccsid[] = "@(#)view_trans77.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int set_back_plane_clipping();
int set_coordinate_system_type();
int set_front_plane_clipping();
int set_output_clipping();
int set_window_clipping();
int set_world_coordinate_matrix_2();
int set_world_coordinate_matrix_3();

int setbackclip_(onoff)
int *onoff;
	{
	return(set_back_plane_clipping(*onoff));
	}

int setcoordsystype_(type)
int *type;
	{
	return(set_coordinate_system_type(*type));
	}

int setfrontclip_(onoff)
int *onoff;
	{
	return(set_front_plane_clipping(*onoff));
	}

int setoutputclip_(onoff)
int *onoff;
	{
	return(set_output_clipping(*onoff));
	}

int setwindowclip_(onoff)
int *onoff;
	{
	return(set_window_clipping(*onoff));
	}

int setworldmatrix2_(f77array)
float *f77array;
	{
	int i;
	float dummy[9], *dptr, *fptr;;

	dptr = dummy;
	for (i = 0; i < 3; i++)
		{
		fptr = f77array + i;
		*dptr++ = *fptr;
		fptr += 3;
		*dptr++ = *fptr;
		fptr += 3;
		*dptr++ = *fptr;
		}
	return(set_world_coordinate_matrix_2(dummy));
	}

int setworldmatrix3_(f77array)
float *f77array;
	{
	int i;
	float dummy[16], *dptr, *fptr;;

	dptr = dummy;
	for (i = 0; i < 4; i++)
		{
		fptr = f77array + i;
		*dptr++ = *fptr;
		fptr += 4;
		*dptr++ = *fptr;
		fptr += 4;
		*dptr++ = *fptr;
		fptr += 4;
		*dptr++ = *fptr;
		}
	return(set_world_coordinate_matrix_3(dummy));
	}

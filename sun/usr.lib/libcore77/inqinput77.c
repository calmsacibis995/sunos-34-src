#ifndef lint
static char sccsid[] = "@(#)inqinput77.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "f77strings.h"

int inquire_echo();
int inquire_echo_position();
int inquire_echo_surface();
int inquire_keyboard();
int inquire_locator_2();
int inquire_stroke();
int inquire_valuator();

int inqecho_(devclass, devnum, echotype)
int *devclass, *devnum, *echotype;
	{
	return(inquire_echo(*devclass, *devnum, echotype));
	}

int inqechoposition_(devclass, devnum, x, y)
int *devclass, *devnum;
float *x, *y;
	{
	return(inquire_echo_position(*devclass, *devnum, x, y));
	}

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

int inqechosurface_(devclass, devnum, surfname)
int *devclass, *devnum;
struct vwsurf *surfname;
	{
	return(inquire_echo_surface(*devclass, *devnum, surfname));
	}

int inqkeyboard_(keynum, bufsize, istr, pos, f77strleng)
int *keynum, *bufsize, *pos, f77strleng;
char *istr;
	{
	char c, *sptr;
	int f, n;

	f = inquire_keyboard(*keynum, bufsize, _core_77string, pos);
	sptr = _core_77string;
	n = 1;
	while ((c = *sptr++) && (n++ <= f77strleng))
		*istr++ = c;
	while (n++ <= f77strleng)
		*istr++ = ' ';
	return(f);
	}

int inqlocator2_(locnum, x, y)
int *locnum;
float *x, *y;
	{
	return(inquire_locator_2(*locnum, x, y));
	}

int inqstroke_(strokenum, bufsize, dist, time)
int *strokenum, *bufsize, *time;
float *dist;
	{
	return(inquire_stroke(*strokenum, bufsize, dist, time));
	}

int inqvaluator_(valnum, init, low, high)
int *valnum;
float *init, *low, *high;
	{
	return(inquire_valuator(*valnum, init, low, high));
	}

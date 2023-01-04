#ifndef lint
static char sccsid[] = "@(#)inqtextent77.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "f77strings.h"

int inquire_text_extent_2();
int inquire_text_extent_3();

int inqtextextent2_(s, dx, dy, f77strleng)
char *s;
float *dx, *dy;
int f77strleng;
	{
	char *sptr;

	f77strleng = f77strleng > MAXLEN ? MAXLEN : f77strleng;
	sptr = _core_77string;
	while (f77strleng--)
		*sptr++ = *s++;
	*sptr = '\0';
	return(inquire_text_extent_2(_core_77string, dx, dy));
	}

int inqtextextent3_(s, dx, dy, dz, f77strleng)
char *s;
float *dx, *dy, *dz;
int f77strleng;
	{
	char *sptr;

	f77strleng = f77strleng > MAXLEN ? MAXLEN : f77strleng;
	sptr = _core_77string;
	while (f77strleng--)
		*sptr++ = *s++;
	*sptr = '\0';
	return(inquire_text_extent_3(_core_77string, dx, dy, dz));
	}

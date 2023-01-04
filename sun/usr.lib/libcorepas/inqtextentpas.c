#ifndef lint
static char sccsid[] = "@(#)inqtextentpas.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

#include "f77strings.h"

int inquire_text_extent_2();
int inquire_text_extent_3();

int inqtextextent2(s, dx, dy)
char *s;
double *dx, *dy;
	{
	char *sptr;
	char pasarg[257];
	int i,strlen;
	float tdx,tdy;

	strlen = 256;
	sptr = s+256;
	while ((*--sptr) == ' ') {strlen--;};
	strncpy (pasarg,s,strlen);
	pasarg[strlen] = '\0';
	i = inquire_text_extent_2(pasarg, &tdx, &tdy);
	*dx=tdx;
	*dy=tdy;
	return(i);
	}

int inqtextextent3(s, dx, dy, dz)
char *s;
double *dx, *dy, *dz;
	{
	char *sptr;
	char pasarg[257];
	int i,strlen;
	float tdx,tdy,tdz;

	strlen = 256;
	sptr = s+256;
	while ((*--sptr) == ' ') {strlen--;};
	strncpy (pasarg,s,strlen);
	pasarg[strlen] = '\0';
	i = inquire_text_extent_3(pasarg, &dx, &dy, &dz);
	*dx=tdx;
	*dy=tdy;
	*dz=tdz;
	return(i);
	}


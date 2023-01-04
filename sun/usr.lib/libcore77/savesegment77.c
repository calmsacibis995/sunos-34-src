#ifndef lint
static char sccsid[] = "@(#)savesegment77.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "f77strings.h"

int restore_segment();
int save_segment();

int restoresegment_(segname, filename, f77strleng)
char *filename;
int *segname, f77strleng;
	{
	char *sptr;

	sptr = filename + f77strleng - 1;
	while (*sptr-- == ' ')
		f77strleng--;
	f77strleng = f77strleng > MAXLEN ? MAXLEN : f77strleng;
	sptr = _core_77string;
	while (f77strleng--)
		*sptr++ = *filename++;
	*sptr = '\0';
	return(restore_segment(*segname, _core_77string));
	}

int savesegment_(segnum, filename, f77strleng)
char *filename;
int *segnum, f77strleng;
	{
	char *sptr;

	sptr = filename + f77strleng - 1;
	while (*sptr-- == ' ')
		f77strleng--;
	f77strleng = f77strleng > MAXLEN ? MAXLEN : f77strleng;
	sptr = _core_77string;
	while (f77strleng--)
		*sptr++ = *filename++;
	*sptr = '\0';
	return(save_segment(*segnum, _core_77string));
	}

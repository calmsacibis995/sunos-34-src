#ifndef lint
static char sccsid[] = "@(#)savesegpas.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

#include "f77strings.h"

int restore_segment();
int save_segment();

int restoresegment(segname, filename)
char *filename;
int segname;
	{
	char *sptr;
	char pasarg[257];
	int i,strlen;

	strlen = 256;
	sptr = filename+256;
	while ((*--sptr) == ' ') {strlen--;};
	strncpy (pasarg,filename,strlen);
	pasarg[strlen] = '\0';
	i = restore_segment(segname,filename);
	return(i);
	}

int savesegment(segnum, filename)
char *filename;
int segnum;
	{
	char *sptr;
	char pasarg[257];
	int i,strlen;

	strlen = 256;
	sptr = filename+256;
	while ((*--sptr) == ' ') {strlen--;};
	strncpy (pasarg,filename,strlen);
	pasarg[strlen] = '\0';
	i = save_segment(segnum,pasarg);
	return(i);
	}

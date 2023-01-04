#ifndef lint
static char sccsid[] = "@(#)segments77.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int delete_all_retained_segments();
int delete_retained_segment();
int rename_retained_segment();

int delallretainsegs()
	{
	return(delete_all_retained_segments());
	}

int delretainsegment(segname)
int *segname;
	{
	return(delete_retained_segment(*segname));
	}

int renameretainseg_(segname, newname)
int *segname, *newname;
	{
	return(rename_retained_segment(*segname, *newname));
	}

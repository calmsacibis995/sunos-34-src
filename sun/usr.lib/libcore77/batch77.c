#ifndef lint
static char sccsid[] = "@(#)batch77.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int begin_batch_of_updates();
int end_batch_of_updates();

int beginbatchupdate()
	{
	return(begin_batch_of_updates());
	}

int endbatchupdate_()
	{
	return(end_batch_of_updates());
	}

#ifndef lint
static char sccsid[] = "@(#)init_termin77.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int initialize_core();
int terminate_core();

int initializecore_(outlev, inlev, dim)
int *outlev, *inlev, *dim;
	{
	return(initialize_core(*outlev, *inlev, *dim));
	}

int terminatecore_()
	{
	return(terminate_core());
	}

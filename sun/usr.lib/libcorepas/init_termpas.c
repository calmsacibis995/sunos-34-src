#ifndef lint
static char sccsid[] = "@(#)init_termpas.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

int initialize_core();
int terminate_core();

int initializecore(outlev, inlev, dim)
int outlev, inlev, dim;
	{
	return(initialize_core(outlev, inlev, dim));
	}

int terminatecore(dummy)
int dummy;
	{
	return(terminate_core());
	}

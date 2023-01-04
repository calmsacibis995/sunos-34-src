#ifndef lint
static char sccsid[] = "@(#)zbuffer77.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

int set_zbuffer_cut();

int setzbuffercut_(xlist, zlist, n)
float xlist[], zlist[];
int *n;
	{
	return(set_zbuffer_cut(xlist, zlist, *n));
	}

#ifndef lint
static char sccsid[] = "@(#)zbufferpas.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

#include "pasarray.h"

#define DEVNAMESIZE 20

typedef struct 	{
    		char screenname[DEVNAMESIZE];
    		char windowname[DEVNAMESIZE];
		int fd;
		int (*dd)();
		int instance;
		int cmapsize;
    		char cmapname[DEVNAMESIZE];
		int flags;
		char *ptr;
		} vwsurf;

int set_zbuffer_cut();

int setzbuffercut(surfname,xlist, zlist, n)
vwsurf *surfname;
double xlist[], zlist[];
int n;
	{
		int i;
		for (i = 0; i < n; ++i) {
			xcoort[i] = xlist[i];
			zcoort[i] = zlist[i];
		}
		return(set_zbuffer_cut(surfname, xcoort, zcoort, n));
	}

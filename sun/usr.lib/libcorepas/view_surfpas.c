#ifndef lint
static char sccsid[] = "@(#)view_surfpas.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

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

int deselect_view_surface();
int initialize_view_surface();
int select_view_surface();
int terminate_view_surface();

int deselectvwsurf(surfname)
vwsurf *surfname;
	{
	return(deselect_view_surface(surfname));
	}

int initializevwsurf(surfname, flag)
vwsurf *surfname;
int flag;
	{
	char *sptr,*wptr,*cptr;
	int i,is,iw,ic;

	is = iw = ic = 0;
	sptr = surfname->screenname;
	while ((*sptr++) != ' ');
	*--sptr = '\0';
	wptr = surfname->windowname;
	while ((*wptr++) != ' ');
	*--wptr = '\0';
	cptr = surfname->cmapname;
	while ((*cptr++) != ' ');
	*--cptr = '\0';
	i = initialize_view_surface(surfname, flag);
	sptr = surfname->screenname;
	while ((*sptr++) != '\0')  is++;
	*--sptr = ' ';
	while (++is < DEVNAMESIZE) *sptr++ = ' ';
	wptr = surfname->windowname;
	while ((*wptr++) != '\0')  iw++;
	*--wptr = ' ';
	while (++iw < DEVNAMESIZE) *wptr++ = ' ';
	cptr = surfname->cmapname;
	while ((*cptr++) != '\0')  ic++;
	*--cptr = ' ';
	while (++ic < DEVNAMESIZE) *cptr++ = ' ';
	return(i);
	}

int selectvwsurf(surfname)
vwsurf *surfname;
	{
	return(select_view_surface(surfname));
	}

int terminatevwsurf(surfname)
vwsurf *surfname;
	{
	return(terminate_view_surface(surfname));
	}

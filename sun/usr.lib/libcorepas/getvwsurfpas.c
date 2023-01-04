#ifndef lint
static char sccsid[] = "@(#)getvwsurfpas.c 1.1 86/09/25 Copyr 1984 Sun Micro";
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

int get_view_surface();
extern char** _argv;

int getviewsurface(viewsurf)
vwsurf *viewsurf;
	{
	    char *sptr,*wptr,*cptr;
	    int is,res;
	    
	    res=get_view_surface(viewsurf,_argv);
	    sptr = viewsurf->screenname;
	    is = 0;
	    while ((*sptr++) != '\0') is++;
	    *--sptr = ' ';
	    while (++is < DEVNAMESIZE) *++sptr = ' ';
	    wptr = viewsurf->windowname;
	    is = 0;
	    while ((*wptr++) != '\0') is++;
	    *--wptr = ' ';
	    while (++is < DEVNAMESIZE) *++wptr = ' ';
	    cptr = viewsurf->cmapname;
	    is = 0;
	    while ((*cptr++) != '\0') is++;
	    *--cptr = ' ';
	    while (++is < DEVNAMESIZE) *++cptr = ' ';
	return(res);
	}


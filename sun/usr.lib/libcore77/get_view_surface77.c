#ifndef lint
static char sccsid[] = "@(#)get_view_surface77.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

int get_view_surface();

#define DEVNAMESIZE 20

struct vwsurf   {
		char screenname[DEVNAMESIZE];
		char windowname[DEVNAMESIZE];
		int windowfd;
		int (*dd)();
		int instance;
		int cmapsize;
		char cmapname[DEVNAMESIZE];
		int flags;
		char **ptr;
                };

extern char **xargv;

int getviewsurface_(surfname)
struct vwsurf *surfname;
	{
	char *sptr;
	int i;

	i = get_view_surface(surfname, xargv);
	sptr = surfname->screenname;
	while (*sptr++)
		;
	--sptr;
	while (sptr < &surfname->screenname[DEVNAMESIZE])
		*sptr++ = ' ';
	sptr = surfname->windowname;
	while (*sptr++)
		;
	--sptr;
	while (sptr < &surfname->windowname[DEVNAMESIZE])
		*sptr++ = ' ';
	sptr = surfname->cmapname;
	while (*sptr++)
		;
	--sptr;
	while (sptr < &surfname->cmapname[DEVNAMESIZE])
		*sptr++ = ' ';
	return(i);
	}

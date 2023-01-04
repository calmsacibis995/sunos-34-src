#ifndef lint
static char sccsid[] = "@(#)view_surface77.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

int deselect_view_surface();
int initialize_view_surface();
int select_view_surface();
int terminate_view_surface();

#define TRUE 1
#define FALSE 0

#define DEVNAMESIZE 20

struct vwsurf	{
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

int deselectvwsurf_(surfname)
struct vwsurf *surfname;
	{
	return(deselect_view_surface(surfname));
	}

int initializevwsurf(surfname, flag)
struct vwsurf *surfname;
int *flag;
	{
	int r;
	char *sptr;
	short blankpad[3];

	sptr = surfname->screenname;
	blankpad[0] = FALSE;
	while (sptr < &surfname->screenname[DEVNAMESIZE])
		{
		if (*sptr == '\0')
			break;
		else if (*sptr == ' ')
			{
			blankpad[0] = TRUE;;
			*sptr = '\0';
			break;
			}
		sptr++;
		}
	sptr = surfname->windowname;
	blankpad[1] = FALSE;
	while (sptr < &surfname->windowname[DEVNAMESIZE])
		{
		if (*sptr == '\0')
			break;
		else if (*sptr == ' ')
			{
			blankpad[1] = TRUE;;
			*sptr = '\0';
			break;
			}
		sptr++;
		}
	sptr = surfname->cmapname;
	blankpad[2] = FALSE;
	while (sptr < &surfname->cmapname[DEVNAMESIZE])
		{
		if (*sptr == '\0')
			break;
		else if (*sptr == ' ')
			{
			blankpad[2] = TRUE;;
			*sptr = '\0';
			break;
			}
		sptr++;
		}
	r = initialize_view_surface(surfname, *flag);
	if (blankpad[0])
		{
		sptr = surfname->screenname;
		while (*sptr++)
			;
		--sptr;
		while (sptr < &surfname->screenname[DEVNAMESIZE])
			*sptr++ = ' ';
		}
	if (blankpad[1])
		{
		sptr = surfname->windowname;
		while (*sptr++)
			;
		--sptr;
		while (sptr < &surfname->windowname[DEVNAMESIZE])
			*sptr++ = ' ';
		}
	if (blankpad[2])
		{
		sptr = surfname->cmapname;
		while (*sptr++)
			;
		--sptr;
		while (sptr < &surfname->cmapname[DEVNAMESIZE])
			*sptr++ = ' ';
		}
	return(r);
	}

int selectvwsurf_(surfname)
struct vwsurf *surfname;
	{
	return(select_view_surface(surfname));
	}

int terminatevwsurf_(surfname)
struct vwsurf *surfname;
	{
	return(terminate_view_surface(surfname));
	}

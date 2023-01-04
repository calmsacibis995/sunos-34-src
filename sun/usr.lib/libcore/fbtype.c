#ifndef lint
static char sccsid[] = "@(#)fbtype.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include "usercore.h"
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include        <sunwindow/window_hs.h>
/*
 * Get the framebuffer type of the current window or raw display
 */
_core_fbtype()
{
    struct fbtype fbtype;
    char name[DEVNAMESIZE];
    int gwfd, fbfd;
    struct screen screen;

	if (we_getgfxwindow(name)) {			/* if no suntools */
		strncpy( name, "/dev/fb", DEVNAMESIZE);
		if ((fbfd = open(name, O_RDWR, 0)) < 0)
			return(-1);
	} else {
		if ((gwfd = open(name, O_RDWR, 0)) < 0)
			return(-1);
		win_screenget(gwfd, &screen);
		if ((fbfd = open(screen.scr_fbname, O_RDWR, 0)) < 0) {
			close(gwfd);
			return(-1);
		}
		close(gwfd);
	}
	ioctl(fbfd, FBIOGTYPE, &fbtype);
	close(fbfd);
	return( fbtype.fb_type);
}

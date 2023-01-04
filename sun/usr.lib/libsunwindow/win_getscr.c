#ifndef lint
static	char sccsid[] = "@(#)win_getscr.c 1.3 87/01/07 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Win_getscr.c: Implement win_getscreen.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sunwindow/rect.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/win_struct.h>

/*
 * Screen creation, inquiry and deletion.
 */
win_screenget(windowfd, screen)
	int 	windowfd;
	struct	screen *screen;
{
	(void)werror(ioctl(windowfd, WINSCREENGET, screen), WINSCREENGET);
	return;
}


#ifndef lint
static	char sccsid[] = "@(#)tool_draw_box.c 1.4 87/01/07 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * This is a WORK-AROUND to rename draw_box as private routine.  This function
 * is an exact copy of _tool_draw_box.  This file was created to catch any demo
 * programs that may have calls to draw_box.  This is NOT a public routine and
 * this file should be removed during the next major release.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>

/*ARGSUSED*/
draw_box(pixwin, op, r, w, color)
	register struct	pixwin *pixwin;
	register int	op;
	register struct rect *r;
	register int	w;
	int	color;
{
	struct rect rectlock;

	/*
	 * Draw top, left, right then bottom.
	 * Note: should be pw_writebackground.
	 */
	rectlock = *r;
	rect_marginadjust(&rectlock, w);
	(void)pw_lock(pixwin, &rectlock);
	(void)pw_writebackground(
	    pixwin, r->r_left, r->r_top, r->r_width, w, op);
	(void)pw_writebackground(pixwin, r->r_left, r->r_top + w,
	    w, r->r_height - 2*w, op);
	(void)pw_writebackground(pixwin, r->r_left + r->r_width - w, r->r_top + w,
	    w, r->r_height - 2*w, op);
	(void)pw_writebackground(pixwin, r->r_left, r->r_top + r->r_height - w,
	    r->r_width, w, op);
	(void)pw_unlock(pixwin);
	return;
}

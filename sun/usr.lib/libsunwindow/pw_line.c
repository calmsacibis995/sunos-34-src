#ifndef lint
static char sccsid[] = "@(#)pw_line.c 1.4 87/01/07";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */
 

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_line.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include "pw_util.h"


pw_line(pw, x0, y0, x1, y1, brush, tex, op)
    int	op;
    register int x0, y0, x1, y1;
    register struct pixwin 	*pw;
    struct pr_brush		*brush;
    Pr_texture			*tex;
{
    register struct pixwin_prlist *prl;
    struct rect 		rdest;
    short left, top;

/* Do std setup */
    left = min(x0, x1);
    top = min(y0, y1);
    PW_SETUP(pw, rdest, DoDraw, left, top,
	max(x0, x1)+1-left, max(y0, y1)+1-top);
/*
 * See if user wants to bypass clipping.
 */
    if (op & PIX_DONTCLIP) {
	(void)pr_line(pw->pw_clipdata->pwcd_prmulti,
	    x0, y0, x1, y1, brush, tex, op);
	goto TryRetained;
    }
/*
 * Loop and clip
 */
    for (prl = pw->pw_clipdata->pwcd_prl;prl;prl = prl->prl_next) {
	(void)pr_line(prl->prl_pixrect,
	    x0-prl->prl_x, y0-prl->prl_y, x1-prl->prl_x, y1-prl->prl_y,
	    brush, tex, op);
    }
TryRetained:
/*
 * Unlock screen
 */
    (void)pw_unlock(pw);
/*
 * Write to retained pixrect if have one.
 */
DoDraw:
    if (pw->pw_prretained) {
	(void)pr_line(pw->pw_prretained,
	    PW_RETAIN_X_OFFSET(pw, x0), PW_RETAIN_Y_OFFSET(pw, y0),
	    PW_RETAIN_X_OFFSET(pw, x1), PW_RETAIN_Y_OFFSET(pw, y1),
	    brush, tex, op);
    }
    return;
} /* pw_line */


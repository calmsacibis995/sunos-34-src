#ifndef lint
static	char sccsid[] = "@(#)pw_curve.c 1.4 87/01/07 Copyr 1986 Sun Micro";
#endif
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * pw_curve2.c curve scan routine for pixwins
 *    built on top of pr_curve_2
 *    see pr_curve2.c for algorithmic details
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/chain.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include "pw_util.h"

pw_curve (pw, x, y, cu, n ,op , color)
    register struct pixwin *pw;
    struct pr_curve cu[];
    register int x,y;
    int n,op,color;
{
    register struct	pixwin_prlist *prl;
    struct	rect rdest;
    struct pr_pos	pos;

    /* Translate dx, dy */
    x = PW_X_OFFSET(pw, x);
    y = PW_Y_OFFSET(pw, y);
    /* Do standard setup */
    PW_SETUP(pw, rdest, DoDraw, 0, 0, pw->pw_pixrect->pr_width, 
        pw->pw_pixrect->pr_height);
    /*
     * See if user wants to bypass clipping.
     */
    if (op & PIX_DONTCLIP) {
    	    pos.x = x; pos.y = y;
	    (void)pr_curve(pw->pw_clipdata->pwcd_prmulti,
	     pos, cu, n ,op , color);
    } else
            for (prl = pw->pw_clipdata->pwcd_prl;prl;prl = prl->prl_next) {
	    	pos.x = x - prl->prl_x; pos.y = y - prl->prl_y;
                (void)pr_curve(prl->prl_pixrect, pos,
			cu, n ,op , color);
            }

   /*
    * Unlock screen
    */
    (void)pw_unlock(pw);

    /*
     * Write to retained pixrect if have one.
     */
DoDraw:
    if (pw->pw_prretained) {
    	    pos.x = PW_RETAIN_X_OFFSET(pw,x); pos.y = PW_RETAIN_Y_OFFSET(pw,y);
	    (void)pr_curve(pw->pw_prretained, pos,
		cu, n ,op , color);
    }

    return;
}

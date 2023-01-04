#ifndef lint
static  char sccsid[] = "@(#)pw_write.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Pw_write.c: Implement the pw_write & pw_writecolor functions
 *	of the pixwin.h interface.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include "pw_util.h"

int	_pw_usereplrop;

pw_write(pw, xw, yw, width, height, op, pr, xr, yr)
	struct	pixwin *pw;
	int	op, xw, yw, width, height;
	struct	pixrect *pr;
	int	xr, yr;
{
	struct	rect rintersect, rdest;

	/*
	 * Construct window relative rectangle that will be writing to
	 */
	rect_construct(&rdest, xw, yw, width, height);
	/*
	 * Magic
	 */
	pw_preaccess(pw, &xw, &yw, &rdest);
	pw_begincliploop(pw, &rdest, &rintersect);
		/*
		 * Write to rintersect portion of pixrect.
		 * All coordinates are relative to the pixrect.
		 */
		if (_pw_usereplrop)
			pr_replrop(pw->pw_pixrect,
			    rintersect.r_left, rintersect.r_top,
			    rintersect.r_width, rintersect.r_height,
			    op, pr, xr+(rintersect.r_left-rdest.r_left),
			    yr+(rintersect.r_top-rdest.r_top));
		else
			pr_rop(pw->pw_pixrect,
			    rintersect.r_left, rintersect.r_top,
			    rintersect.r_width, rintersect.r_height,
			    op, pr, xr+(rintersect.r_left-rdest.r_left),
			    yr+(rintersect.r_top-rdest.r_top));
	/*
	 * More magic
	 */
	pw_endcliploop(&rdest, &rintersect);
	pw_postaccess(pw, &xw, &yw);
	/*
	 * Write to retained pixrect if have one.
	 */
	if (pw->pw_prretained) {
	    pr_rop(pw->pw_prretained, xw, yw, width, height, op, pr, xr, yr);
	}
	return;
}

pw_writepattern(pw, xw, yw, width, height, op, pr, xr, yr)
	struct	pixwin *pw;
	int	op, xw, yw, width, height;
	struct	pixrect *pr;
	int	xr, yr;
{
	_pw_usereplrop = TRUE;
	pw_write(pw, xw, yw, width, height, op, pr, xr, yr);
	_pw_usereplrop = FALSE;
	return;
}

pw_writecolor(pw, xw, yw, width, height, op, cms_index)
	struct	pixwin *pw;
	int	op, xw, yw, width, height;
	int	cms_index;
{
#ifdef notdef
	/*
	 * Remember old cms
	 */
	struct	colormapseg *cms = &pw->pw_pixrect->pd_cms;
	struct	colormapseg cmssaved;

	cmssaved = *cms;
	/*
	 * Adjust cms values to give new background, i.e., value at cms_addr.
	 */
	cms->cms_addr += min(cms_index, cms->cms_size-1);
	cms->cms_size = cms->cms_addr-cmssaved.cms_addr;
	if (pw->pw_pdretained)
		pw->pw_pdretained->pd_cms = *cms;
	/*
	 * Write background using new background
	 */
	pw_writebackground(pw, xw, yw, width, height, op);
	/*
	 * Restore old cms values
	 */
	*cms = cmssaved;
	if (pw->pw_pdretained)
		pw->pw_pdretained->pd_cms = cmssaved;
#endif notdef
	return;
}


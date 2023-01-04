#ifndef lint
static	char sccsid[] = "@(#)csr_change.c 1.9 87/03/17 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Character screen operations (except size change and init).
 */
#include <sys/types.h>
#include <sys/time.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <suntool/tool_struct.h>
#include <suntool/ttysw.h>
#include "charimage.h"
#include "charscreen.h"
#undef CTRL
#include "ttyansi.h"

extern	wfd;
extern	struct	pixwin *csr_pixwin;
extern	int	cursor;	/* NOCURSOR, UNDERCURSOR, BLOCKCURSOR */
extern  void    pos();

static	int caretx, carety;
static	short charcursx, charcursy;

static	int boldstyle;

struct timeval ttysw_bell_tv = {0, 100000}; /* 1/10 second */

ttysw_setboldstyle(new_boldstyle)
{
	if (new_boldstyle > TTYSW_BOLD_MAX || new_boldstyle < TTYSW_BOLD_NONE)
		boldstyle = TTYSW_BOLD_INVERT;
	else
		boldstyle = new_boldstyle;
	return boldstyle;
}

ttysw_getboldstyle()
{
	return boldstyle;
}

ttysw_setleftmargin(left_margin)
{
	chrleftmargin = left_margin > 0 ? left_margin : 0;
}

ttysw_getleftmargin()
{
	return chrleftmargin;
}

pstring(s, col, row, op)
	register char *s;
	register int col, row;
	int op; /* PIX_SRC | PIX_DST (faster), or PIX_SRC (safer) */
{
	int	bold = 0;
	register char *cp;

	if (delaypainting) {
		if (row == bottom)
			/*
			 * Reached bottom of screen so end delaypainting.
			 */
			(void)pdisplayscreen(1);
		return;
	}
	if (s==0)
		return;
	if (*s & BOLDBIT) {
		bold = 1;
		for (cp = s;*cp;cp++)
			*cp &= ~BOLDBIT;
	}
	if ((!bold) || (boldstyle == TTYSW_BOLD_INVERT))
		(void)pw_text(csr_pixwin,
                   col_to_x(col)-pixfont->pf_char[*s].pc_home.x,
                   row_to_y(row)-pixfont->pf_char[*s].pc_home.y,
                  (bold)? PIX_NOT(PIX_SRC): op, pixfont, s);
	else {
		(void)pw_text(csr_pixwin,
                   col_to_x(col)-pixfont->pf_char[*s].pc_home.x,
                   row_to_y(row)-pixfont->pf_char[*s].pc_home.y,
                   op, pixfont, s);
		if (boldstyle & TTYSW_BOLD_OFFSET_X)
                      (void)pw_text(csr_pixwin,
                         col_to_x(col)-pixfont->pf_char[*s].pc_home.x+1,
                         row_to_y(row)-pixfont->pf_char[*s].pc_home.y,
                         PIX_SRC|PIX_DST, pixfont, s);
		if (boldstyle & TTYSW_BOLD_OFFSET_Y)
                      (void)pw_text(csr_pixwin,
                         col_to_x(col)-pixfont->pf_char[*s].pc_home.x,
                         row_to_y(row)-pixfont->pf_char[*s].pc_home.y+1,
                         PIX_SRC|PIX_DST, pixfont, s);
		if (boldstyle & TTYSW_BOLD_OFFSET_XY)
                      (void)pw_text(csr_pixwin,
                         col_to_x(col)-pixfont->pf_char[*s].pc_home.x+1,
                         row_to_y(row)-pixfont->pf_char[*s].pc_home.y+1,
                         PIX_SRC|PIX_DST, pixfont, s);
	}
	if (bold)
		for (cp = s;*cp;cp++)
			*cp |= BOLDBIT;
}

pclearline(fromcol, tocol, row)
	int fromcol, tocol, row;
{
	if (delaypainting)
		return;
	(void)pw_writebackground(csr_pixwin, col_to_x(fromcol), row_to_y(row),
	    col_to_x(tocol)-col_to_x(fromcol), chrheight, PIX_CLR);
}

pcopyline(tocol, fromcol, count, row)
	int fromcol, tocol, count, row;
{
        int	pix_width = (count * chrwidth);
        
	if (delaypainting)
		return;
	(void)pw_copy(csr_pixwin, col_to_x(tocol), row_to_y(row),
	    pix_width, chrheight, PIX_SRC,
	    csr_pixwin, col_to_x(fromcol), row_to_y(row));
	(void)prepair(wfd, &csr_pixwin->pw_fixup);
	(void)rl_free(&csr_pixwin->pw_fixup);
}

pclearscreen(fromrow, torow)
	int fromrow, torow;
{
	if (delaypainting)
		return;
	(void)pw_writebackground(csr_pixwin, col_to_x(left), row_to_y(fromrow),
	    winwidthp, row_to_y(torow-fromrow), PIX_CLR);
}

pcopyscreen(fromrow, torow, count)
	int fromrow, torow, count;
{
	if (delaypainting)
		return;
	(void)pw_copy(csr_pixwin, col_to_x(left), row_to_y(torow), winwidthp,
	    row_to_y(count), PIX_SRC,
	    csr_pixwin, col_to_x(left), row_to_y(fromrow));
	(void)prepair(wfd, &csr_pixwin->pw_fixup);
	(void)rl_free(&csr_pixwin->pw_fixup);
}

pdisplayscreen(dontrestorecursor)
	int	dontrestorecursor;
{
	delaypainting = 0;
	/*
	 * refresh the entire image.
	 */
	if (csr_pixwin->pw_prretained) {
		Rectlist rl;
		Rect r;

		rect_construct(&r, 0, 0, winwidthp, winheightp);
		(void)rl_initwithrect(&r, &rl);
		(void)prepair(wfd, &rl);
		(void)rl_free(&rl);
	} else
		(void)prepair(wfd, &csr_pixwin->pw_clipdata->pwcd_clipping);
	if (!dontrestorecursor)
		/*
		 * The following has effect of restoring cursor.
		 */
		(void)removeCursor();
}

/* ARGSUSED */
prepair(windowfd, rlp)
	int	windowfd;
	struct	rectlist *rlp;
{
	struct	rectnode *rnp;
	struct	rect rect;

	rl_rectoffset(rlp, &rlp->rl_bound, &rect);
	(void)pw_lock(csr_pixwin, &rect);
	for (rnp = rlp->rl_head; rnp; rnp = rnp->rn_next) {
		register int row, colstart, blanks, bold, colfirst;
		register char *strstart, *strfirst;
		int	toprow, botrow, leftcol;
		char	csave;

		colfirst = 0;	/* make LINT shut up */
		rl_rectoffset(rlp, &rnp->rn_rect, &rect);
		(void)pw_writebackground(csr_pixwin, rect.r_left, rect.r_top,
		    rect.r_width, rect.r_height, PIX_CLR);
		toprow = y_to_row(rect.r_top);
		botrow = min(bottom, y_to_row(rect_bottom(&rect))+1);
		leftcol = x_to_col(rect.r_left);
		for (row = toprow; row < botrow; row++) {
		    colstart = leftcol;
		    if (length(image[row]) > leftcol) {
			strfirst = (caddr_t) 0;
			bold = 0;
			blanks = 1;
			for (strstart = image[row]+leftcol;*strstart;
			    strstart++, colstart++) {
				/*
				 * Find beginning of bold string
				 */
				if ((*strstart & BOLDBIT) && !bold) {
					bold = 1;
					goto Flush;
				/*
				 * Find end of bold string
				 */
				} else if (bold && (~(*strstart) & BOLDBIT)) {
					bold = 0;
					goto Flush;
				/*
				 * Find first non-blank char
				 */
				} else if (blanks && ((*strstart&0x7f) != ' '))
					goto Flush;
				else
					continue;
Flush:
				if (strfirst != (caddr_t) 0) {
					csave = *strstart;
					*strstart = '\0';
					(void)pstring(strfirst, colfirst, row,
					    PIX_SRC | PIX_DST);
					*strstart = csave;
				}
				colfirst = colstart;
				strfirst = strstart;
				blanks = 0;
			}
			if (strfirst != (caddr_t) 0)
				(void)pstring(strfirst, colfirst, row,
				    PIX_SRC | PIX_DST);
		    }
		}
	}
	(void)pw_unlock(csr_pixwin);
}

drawCursor(yChar, xChar)
{
	charcursx = xChar;
	charcursy = yChar;
	caretx = col_to_x(xChar);
	carety = row_to_y(yChar);
	if (delaypainting || cursor == NOCURSOR)
		return;
	(void)pw_writebackground(csr_pixwin,
	    caretx, carety, chrwidth, chrheight, PIX_NOT(PIX_DST));
	if (cursor & LIGHTCURSOR) {
		(void)pw_writebackground(csr_pixwin,
		    caretx-1, carety-1, chrwidth+2, chrheight+2, PIX_NOT(PIX_DST));
		(void) pos(xChar, yChar);   
	}
}

removeCursor()
{
	if (delaypainting || cursor == NOCURSOR)
		return;
	(void)pw_writebackground(csr_pixwin,
	    caretx, carety, chrwidth, chrheight, PIX_NOT(PIX_DST));
	if (cursor & LIGHTCURSOR) {
		(void)pw_writebackground(csr_pixwin,
		    caretx-1, carety-1, chrwidth+2, chrheight+2, PIX_NOT(PIX_DST));
	}
}

saveCursor()
{
	(void)removeCursor();
}

restoreCursor()
{
	(void)removeCursor();
}

screencomp()
{
}

blinkscreen()
{
	struct timeval	now;
static	struct timeval	lastblink;

	(void)gettimeofday(&now, (struct timezone *) 0);
	if (now.tv_sec - lastblink.tv_sec > 1)
		(void)win_bell(csr_pixwin->pw_windowfd, ttysw_bell_tv, csr_pixwin);
	lastblink = now;
}

pselectionhilite(r)
	struct	rect *r;
{
	struct rect rectlock;

	rectlock = *r;
	rect_marginadjust(&rectlock, 1);
	(void)pw_lock(csr_pixwin, &rectlock);
	(void)pw_writebackground(csr_pixwin, r->r_left, r->r_top,
	    r->r_width, r->r_height, PIX_NOT(PIX_DST));
	(void)pw_unlock(csr_pixwin);
}

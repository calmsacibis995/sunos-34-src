#ifndef lint
static  char sccsid[] = "@(#)cim_size.c 1.8 87/01/07 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Character image initialization, destruction and size changing routines
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <stdio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_notify.h>
#include <suntool/tool.h>
	/* tool.h must be before any indirect include of textsw.h */
#include "ttysw_impl.h"
#include "ttyansi.h"
#include "charimage.h"
#include "charscreen.h"

static	int maxright, maxbottom;
static	char *imagecharsfree;

/*
 * Initialize initial character image.
 */
imageinit(window_fd)
	int     window_fd;
{
	char	**imagealloc();
	int	 maximagewidth, maximageheight;

	if (wininit(window_fd, &maximagewidth, &maximageheight) == 0)
		return(0);
	top = left = 0;
	curscol = left;
	cursrow = top;
	maxright = x_to_col(maximagewidth);
	if (maxright > 255)
		maxright = 255;		/* line length is stored in a byte */
	maxbottom = y_to_row(maximageheight);
	image = imagealloc();
	(void)pclearscreen(0, bottom+1);	/* +1 to get remnant at bottom */
	return(1);
}

/*
 * Allocate character image.
 */
char	**
imagealloc()
{
	register char	**newimage;
	register int	i;
	int	 	nchars;
	register char	*line;
	extern char	*calloc();

	/*
	 * Determine new screen dimensions
	 */
	right = x_to_col(winwidthp);
	bottom = y_to_row(winheightp);
	/*
	 * Ensure has some non-zero dimension
	 */
	if (right < 1)
		right = 1;
	if (bottom < 1)
		bottom = 1;
	/*
	 * Bound new screen dimensions
	 */
	right = (right < maxright)? right: maxright;
	bottom = (bottom < maxbottom)? bottom: maxbottom;
	/*
	 * Let pty set terminal size
	 */
	(void)ttynewsize(right, bottom);
	/*
	 * Allocate line array and character storage
	 */
	nchars = right * bottom;
	newimage = (char **)LINT_CAST(calloc(1, (unsigned)(bottom * sizeof (char *))));
	line = (char *)calloc(1, (unsigned)(nchars + 2 * bottom));
	for( i = 0; i < bottom; i++ ) {
		newimage[i] = line + 1;
		setlinelength(newimage[i], 0);
		line += right + 2;
	}
	/*
	 * Remember allocation pointer so can free correct one.
	 */
	imagecharsfree = newimage[0]-1;
	return(newimage);
}

/*
 * Free character image.
 */
imagefree(imagetofree, imagecharstofree)
	char	**imagetofree, *imagecharstofree;
{
	free((char *)imagecharstofree);
	free((char *)imagetofree);
}

/*
 * Called when screen changes size.  This will let lines get longer
 * (or shorter perhaps) but won't re-fold older lines.
 */
imagerepair(ttysw)
	Ttysw	*ttysw;
{
	char		**oldimage = image;
	char		*oldimagecharsfree = imagecharsfree;
	char		newline = CTRL(J);
	char		cursorflushleft = CTRL(M);
	char		clearscreen = CTRL(L);
	extern int	boldify;
	register int	l;
	int		oldbottom = bottom;
	int		topstart;
	int		clrbottom;
	int		boldsave = boldify;
	int		freeze_save;

	ttysw_save_state();
	boldify = 0;
	/*
	 * Get new image and image description
	 */
	image = imagealloc();
	/*
	 * Clear max of old/new screen (not image).
	 */
	(void)saveCursor();
	clrbottom = (oldbottom < bottom)? bottom+2: oldbottom+2;
	(void)pclearscreen(0, clrbottom);
	(void)restoreCursor();
	/*
	 * Find out where last line of text is (actual oldbottom).
	 */
	for (l = oldbottom;l > top;l--) {
		if (length(oldimage[l-1])) {
			oldbottom = l;
			break;
		}
	}
	/*
	 * Try to perserve bottom (south west gravity) text.
	 * This wouldn't work well for vi and other programs that
	 * know about the size of the terminal but aren't notified of changes.
	 * However, it should work in many cases  for straight tty programs
	 * like the shell.
	 */
	if (oldbottom > bottom)
		topstart = oldbottom-bottom;
	else
		topstart = 0;
	/*
	 * Feed same characters back into ttyansi
	 */
	freeze_save = ttysw->ttysw_frozen;
	ttysw->ttysw_frozen = 0;
	ttysw->ttysw_lpp = 0;
	(void)gfxstring(&clearscreen, 1);
	/*
	 * Note: hangs if don't turn off delaypainting
	 * [set by gfxstring(&clearscreen,1)].
	 * I don't know why but I need a quick fix.
	delaypainting = 0;
	 */
	for (l = topstart;l < oldbottom;l++) {
		register int	sl = strlen(oldimage[l]);
#ifdef	DEBUG_LINELENGTH_WHEN_WRAP
if (sl != length(oldimage[l]))
	printf("real %D saved %D, l %D, oldbottom %D bottom %D\n", sl, length(oldimage[l]), l, oldbottom, bottom);
#endif
		if (sl < right) {
			if (sl)
				(void)gfxstring(oldimage[l], sl);
			if (l+1 < oldbottom) {
				(void)gfxstring(&newline, 1);
				(void)gfxstring(&cursorflushleft, 1);
			}
		} else
			(void)gfxstring(oldimage[l], right);
	}
	ttysw->ttysw_frozen = freeze_save;
	(void)imagefree(oldimage, oldimagecharsfree);
	boldify = boldsave;
	if (delaypainting)
		(void)pdisplayscreen(0);
	ttysw_restore_state();
}


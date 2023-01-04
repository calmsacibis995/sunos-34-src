#ifndef lint
static	char sccsid[] = "@(#)canvasflash.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <suntool/gfx_hs.h>

main(argc, argv)
	int argc;
	char **argv;
{
	int op;
	struct gfxsubwindow *gfx;


	/* initialization */
	if ((gfx = gfxsw_init(0, argv)) == NULL) {
		fprintf(stderr, "Unable to open graphics subwindow.\n");
		exit(1);
	}

	pw_writebackground(gfx->gfx_pixwin, 0, 0,
	    gfx->gfx_rect.r_width, gfx->gfx_rect.r_height, PIX_CLR);

	/* display loop */
	while (gfx->gfx_reps--) {

		/* check to see if window has changed size or been exposed */
		if (gfx->gfx_flags & GFX_DAMAGED)
			gfxsw_handlesigwinch(gfx);

		/* screen has been corrupted and must be redrawn */
		if (gfx->gfx_flags & GFX_RESTART) {
			gfx->gfx_flags &= ~GFX_RESTART;
			pw_writebackground(gfx->gfx_pixwin, 0, 0,
			    gfx->gfx_rect.r_width, gfx->gfx_rect.r_height,
			    PIX_CLR);
		}

		/* change raster operation between each iteration */
		op = (gfx->gfx_reps % 2) ? PIX_SRC : PIX_NOT(PIX_SRC);

		/* sample pw_* calls */
		pw_vector(gfx->gfx_pixwin, 5, 5, 5, 100, op, 1);
		pw_writebackground(gfx->gfx_pixwin, 25, 25, 75, 75, op);
		pw_text(gfx->gfx_pixwin, 5, 125, op, 
		    NULL, "This is a string written with pw_text.");
		sleep(1);
	}

	/* clean up */
	gfxsw_done(gfx);
}

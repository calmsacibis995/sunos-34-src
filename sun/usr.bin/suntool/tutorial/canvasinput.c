#ifndef lint
static	char sccsid[] = "@(#)canvasinput.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <suntool/gfx_hs.h>
#include <suntool/menu.h>

extern	struct menuitem *menu_display();

static	struct gfxsubwindow *gfx;

 /* Menu Definition */
static	struct menuitem menu_items[] = {
	{MENU_IMAGESTRING,	"vector",	(caddr_t)'v'},
	{MENU_IMAGESTRING,	"square",	(caddr_t)'s'},
	{MENU_IMAGESTRING,	"text",		(caddr_t)'t'},
	{MENU_IMAGESTRING,	"clear", 	(caddr_t)'c'},
	{MENU_IMAGESTRING,	"quit", 	(caddr_t)'q'},
};
static	struct menu menu_body = {
	MENU_IMAGESTRING, "Commands",
	sizeof(menu_items) / sizeof(struct menuitem), menu_items,
	(struct menu *)NULL, (caddr_t)NULL
};
static	struct menu *menu_ptr = &menu_body;

main(argc, argv)
	int argc;
	char **argv;
{
	int canvas_selected();
	struct inputmask im;

	/* Initialization */
	if ((gfx = gfxsw_init(0, argv)) == NULL) {
		fprintf(stderr, "Unable to open graphics subwindow.\n");
		exit(1);
	}
	input_imnull(&im);
	im.im_flags |= IM_ASCII | IM_NEGEVENT;
	win_setinputcodebit(&im, MENU_BUT);
	gfxsw_setinputmask(gfx,
	    &im, (struct inputmask *)NULL, WIN_NULLLINK, 1, 1);
	gfxsw_getretained(gfx);
 
	/* Notification Manager */
	gfxsw_select(gfx, canvas_selected, 0, 0, 0, (struct timeval *)NULL);
 
	/* Cleanup */
	gfxsw_done(gfx);
}

/* Notification Handling */
canvas_selected(gfx, ibits, obits, ebits, timer)
	struct	gfxsubwindow *gfx;
	int	*ibits, *obits, *ebits;
	struct	timeval **timer;
{
	struct menuitem *mi;
	struct inputevent ie;

	if (gfx->gfx_flags & GFX_RESTART) {
		gfx->gfx_flags &= ~GFX_RESTART;
		pw_writebackground(gfx->gfx_pixwin, 0, 0,
		    gfx->gfx_rect.r_width, gfx->gfx_rect.r_height, PIX_CLR);
	}
	if (*ibits & (1 << gfx->gfx_windowfd)) {
		if (input_readevent(gfx->gfx_windowfd, &ie)) {
			perror("canvasinput");
			exit(1);
		}
		if (ie.ie_code == MENU_BUT && win_inputposevent(&ie) &&
		    (mi = menu_display(&menu_ptr, &ie, gfx->gfx_windowfd)))
			ie.ie_code = (short) mi->mi_data;
		switch (ie.ie_code) {
		case 'v':
			pw_vector(gfx->gfx_pixwin, 5, 5, 5, 100, PIX_SET, 0);
			break;
		case 's':
			pw_writebackground(gfx->gfx_pixwin,
			    25, 25, 75, 75, PIX_SET);
			break;
		case 't':
			pw_text(gfx->gfx_pixwin, 5, 125, PIX_SRC, 
			    (struct pixfont *)NULL,
			    "This is a string written with pw_text.");
			break;
		case 'c':
			pw_writebackground(gfx->gfx_pixwin, 0, 0,
			    gfx->gfx_rect.r_width, gfx->gfx_rect.r_height,
			    PIX_CLR);
			break;
		case 'q':
			gfxsw_selectdone(gfx);
			break;
		default:
			gfxsw_inputinterrupts(gfx, &ie);
		}
	}
	*ibits = *obits = *ebits = 0;
}

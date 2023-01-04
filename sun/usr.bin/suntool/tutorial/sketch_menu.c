#ifndef lint
static	char sccsid[] = "@(#)sketch_menu.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <suntool/tool_hs.h>
#include <suntool/gfxsw.h>
#include <suntool/menu.h>

struct tool		*tool;
struct toolsw		*sgsw, *pgsw;
struct gfxsubwindow	*sketch, *proof;
int			sigwinched(), do_select();

int sqsiz = 16;
int psiz = 40;

#define CLEAR_BUTTON	(caddr_t)'c'
#define BLACKEN_BUTTON	(caddr_t)'b'

struct menuitem menu_items[] = {
	{ MENU_IMAGESTRING, "clear",   CLEAR_BUTTON },
	{ MENU_IMAGESTRING, "blacken", BLACKEN_BUTTON }
};
struct menu menu_body = {
	MENU_IMAGESTRING, "Commands",
	sizeof(menu_items) / sizeof(struct menuitem),
	menu_items, NULL, NULL
};
struct menu *menu_ptr = &menu_body;

main(argc, argv)	/* tool with sketchpad and pixel image */
	int argc;
	char *argv[];
{
	struct inputmask mask;
	struct pixfont *font;
	int wsiz;

	wsiz = sqsiz * psiz;
	font = pw_pfsysopen();
	if ((tool = tool_make(
	    WIN_LABEL,  argv[0], 
	    WIN_TOP,    100, 
	    WIN_LEFT,   100,
	    WIN_WIDTH,  wsiz + psiz + (TOOL_BORDERWIDTH * 3), 
	    WIN_HEIGHT, wsiz + (font->pf_defaultsize.y + 2) + TOOL_BORDERWIDTH,
	    0)) == NULL) {
		fputs("Can't make tool\n", stderr);
		exit(1);
	}

	/* setup sketch subwindow (including select routine) */
	if ((sgsw = gfxsw_createtoolsubwindow(tool, "", wsiz,wsiz,0)) == NULL) {
		fputs("Can't create sketch graphics subwindow\n", stderr);
		exit(1);
	}
	sketch = (struct gfxsubwindow *)sgsw->ts_data;
	gfxsw_getretained(sketch);
	sgsw->ts_io.tio_selected = do_select;

	/* setup mouse buttons for sketch subwindow (can't OR them together) */
	input_imnull(&mask);
	win_setinputcodebit(&mask, MS_LEFT);
	win_setinputcodebit(&mask, MS_MIDDLE);
	win_setinputcodebit(&mask, MS_RIGHT);
	win_setinputcodebit(&mask, LOC_MOVEWHILEBUTDOWN);
	win_setinputmask(sketch->gfx_windowfd, &mask,
	    (struct inputmask *)NULL, WIN_NULLLINK);

	/* setup proof subwindow */
	if ((pgsw = gfxsw_createtoolsubwindow(tool,"",psiz,psiz,0)) == NULL) {
		fputs("Can't create proof graphics subwindow\n", stderr);
		exit(1);
	}
	proof = (struct gfxsubwindow *)pgsw->ts_data;
	gfxsw_getretained(proof);

	/* normal boilerplate */
	signal(SIGWINCH, sigwinched);
	tool_install(tool);				/* in window tree */
	tool_select(tool, 0);				/* loop for input */
	tool_destroy(tool);				/* tool clean up */
}

sigwinched()	/* note window size change and damage repair signal */
{
	tool_sigwinch(tool);
}

do_select(sw, ibits, obits, ebits, timer)	/* respond to user input */
	caddr_t sw;
	int *ibits, *obits, *ebits;
	struct timeval **timer;
{
	struct inputevent ie;
	static drawval;
	int x, y;

	input_readevent(sketch->gfx_windowfd, &ie);
	/* if button up, else button down */
	if (win_inputnegevent(&ie))
		goto done;
	else if (ie.ie_code == MS_LEFT)
		drawval = PIX_SET;
	else if (ie.ie_code == MS_MIDDLE)
		drawval = PIX_CLR;
	else if (ie.ie_code == MS_RIGHT) {
		do_menu(&ie);
		goto done;
	}
	/* paint rectangle and dot */
	x = ie.ie_locx - (ie.ie_locx % sqsiz);
	y = ie.ie_locy - (ie.ie_locy % sqsiz);
	pw_writebackground(sketch->gfx_pixwin, x, y, sqsiz-1, sqsiz-1, drawval);
	pw_put(proof->gfx_pixwin, x/sqsiz, y/sqsiz, drawval==PIX_SET ? 1 : 0);
done:
	*ibits = *obits = *ebits = 0;
}

do_menu(ie)		/* perform requests issued from pop-up menu */
	struct inputevent *ie;
{
	struct menuitem *mi;
	struct rect r;

	if (mi = menu_display(&menu_ptr, ie, sketch->gfx_windowfd)) {
		win_getsize(sketch->gfx_windowfd, &r);
		if (mi->mi_data == CLEAR_BUTTON) {
			pw_writebackground(sketch->gfx_pixwin,
			    0, 0, r.r_width, r.r_height, PIX_CLR);
			pw_writebackground(proof->gfx_pixwin,
			    0, 0, psiz, psiz, PIX_CLR);
		} else if (mi->mi_data == BLACKEN_BUTTON) {
			pw_writebackground(sketch->gfx_pixwin,
			    0, 0, r.r_width, r.r_height, PIX_SET);
			pw_writebackground(proof->gfx_pixwin,
			    0, 0, psiz, psiz, PIX_SET);
		}
	}
}

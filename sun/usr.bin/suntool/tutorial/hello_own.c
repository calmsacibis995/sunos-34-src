#ifndef lint
static	char sccsid[] = "@(#)hello_own.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <suntool/tool_hs.h>
#include <suntool/msgsw.h>

struct tool *tool;
struct toolsw *my_sw;
struct pixwin *my_pixwin;
struct rect my_rect;
int sigwinched(), my_sigwinch();

main(argc, argv)	/* print "Hello world!" in user defined subwindow */
	int argc;
	char *argv[];
{
	if ((tool = tool_make(WIN_LABEL, argv[0], 0)) == NULL) {
		fputs("Can't make tool\n", stderr);
		exit(1);
	}

	/* Create a vanilla subwindow */
	if ((my_sw = tool_createsubwindow(tool, "", TOOL_SWEXTENDTOEDGE,
	    TOOL_SWEXTENDTOEDGE, "Hello world!", NULL)) == NULL) {
		fputs("Can't create subwindow\n", stderr);
		exit(1);
	}

	/* Open a pixwin with which to draw in the subwindow */
	if ((my_pixwin = pw_open(my_sw->ts_windowfd)) == NULL) {
		fputs("Can't open pixwin\n", stderr);
		exit(1);
	}

	/* Remember subwindow size so that we can notice size changes */
	win_getsize(my_sw->ts_windowfd, &my_rect);

	/* Register a SIGWINCH handler by setting up a function to call */
	my_sw->ts_io.tio_handlesigwinch = my_sigwinch;

	/* Normal boilerplate */
	signal(SIGWINCH, sigwinched);	/* trap window change signal */
	tool_install(tool);		/* install tool in window tree */
	tool_select(tool, 0);		/* main loop to read input */
	tool_destroy(tool);		/* after user exits, clean up */
}

sigwinched()	/* note window size change and damage repair signal */
{
	tool_sigwinch(tool);
}

my_sigwinch(sw)	/* deal with subwindow size change and damage repair */
	caddr_t sw;
{
	struct rect nrect;

	/* Determine current size of subwindow */
	win_getsize(my_sw->ts_windowfd, &nrect);
	/* Prepare pixwin for damage repair */
	pw_damaged(my_pixwin);
	/* If the size has changed */
	if (my_rect.r_width != nrect.r_width ||
	    my_rect.r_height != nrect.r_height)
		/* Set pixwin clipping to be all visible portion of subwindow */
		pw_donedamaged(my_pixwin);
	/* Clear pixwin */
	pw_writebackground(my_pixwin, 0, 0, nrect.r_width, nrect.r_height,
	    PIX_CLR);
	/* Draw text, roughly vertically centered */
	pw_text(my_pixwin, 10, nrect.r_height/2, PIX_SRC, NULL, "Hello world!");
	/* Make sure call pw_donedamaged if haven't above */
	if (my_rect.r_width == nrect.r_width &&
	    my_rect.r_height == nrect.r_height)
		pw_donedamaged(my_pixwin);
	else
		/* Remember new subwindow size */
		my_rect = nrect;
}


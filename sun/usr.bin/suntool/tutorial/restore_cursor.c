#ifndef lint
static	char sccsid[] = "@(#)restore_cursor.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <suntool/tool_hs.h>

struct tool *tool;
int sigwinched(), changecursor();

DEFINE_CURSOR(crosshair, 7, 7, PIX_SRC | PIX_DST,
	0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0000, 0xFC7E,
	0x0000, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0000);
DEFINE_CURSOR(oldcursor, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

main(argc, argv)	/* create window with 5 second crosshair cursor */
	int argc;
	char *argv[];
{
	if ((tool = tool_make(WIN_LABEL, argv[0], 0)) == NULL) {
		fputs("Can't make tool\n", stderr);
		exit(1);
	}
	signal(SIGWINCH, sigwinched);
	tool_install(tool);				/* in window tree */
 	win_getcursor(tool->tl_windowfd, &oldcursor);	/* save cursor */
	win_setcursor(tool->tl_windowfd, &crosshair);	/* change cursor */
        signal(SIGALRM, changecursor);			/* signal routine */
	alarm(5);					/* alarm in 5 seconds */
	tool_select(tool, 0);				/* loop for input */
	tool_destroy(tool);				/* tool clean up */
}

changecursor()		/* change cursor back to original image */
{
	win_setcursor(tool->tl_windowfd, &oldcursor);
}

sigwinched()	/* note window size change and damage repair signal */
{
	tool_sigwinch(tool);
}

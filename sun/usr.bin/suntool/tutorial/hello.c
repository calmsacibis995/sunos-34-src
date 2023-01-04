#ifndef lint
static	char sccsid[] = "@(#)hello.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <suntool/tool_hs.h>
#include <suntool/msgsw.h>

struct tool *tool;
int sigwinched();

main(argc, argv)	/* print "Hello world!" in message subwindow */
	int argc;
	char *argv[];
{
	struct toolsw *msg_sw;

	if ((tool = tool_make(WIN_LABEL, argv[0], 0)) == NULL) {
		fputs("Can't make tool\n", stderr);
		exit(1);
	}
	if ((msg_sw = msgsw_createtoolsubwindow(tool, "", 
	    TOOL_SWEXTENDTOEDGE, TOOL_SWEXTENDTOEDGE,
	    "Hello world!", NULL)) == NULL) {
		fputs("Can't create msgsw\n", stderr);
		exit(1);
	}
	signal(SIGWINCH, sigwinched);	/* trap window change signal */
	tool_install(tool);		/* install tool in window tree */
	tool_select(tool, 0);		/* main loop to read input */
	tool_destroy(tool);		/* after user exits, clean up */
}

sigwinched()	/* note window size change and damage repair signal */
{
	tool_sigwinch(tool);
}

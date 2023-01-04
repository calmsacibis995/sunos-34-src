#ifndef lint
static	char sccsid[] = "@(#)status.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <suntool/tool_hs.h>
#include <suntool/panel.h>
#include <sys/resource.h>

struct tool	*tool;
struct toolsw	*control_panel_sw, *output_panel_sw;
Panel		control_panel, output_panel;
Panel_item	output_item, date_item, rusage_item;
int		sigwinched(), date_proc(), rusage_proc();

main(argc, argv)	
	int argc;
	char *argv[];
{

	if ((tool = tool_make(WIN_LABEL, argv[0], 0)) == NULL) {
	        fputs("Can't make tool\n", stderr);
		exit(1);
	}

	/* setup control panel */
	if ((control_panel_sw = panel_create(tool, 0)) == NULL) {
		fputs("Can't create control_panel\n", stderr);
		exit(1);
	}
	control_panel = control_panel_sw->ts_data;
	date_item =   panel_create_item(control_panel, PANEL_BUTTON,
	    PANEL_LABEL_STRING, "Date", 
	    PANEL_NOTIFY_PROC,  date_proc, 
	    0);
	rusage_item = panel_create_item(control_panel, PANEL_BUTTON,
	    PANEL_LABEL_STRING, "Resources",
	    PANEL_NOTIFY_PROC,  rusage_proc, 
	    0);
	panel_fit_height(control_panel);

	/* setup output panel */
	if ((output_panel_sw = panel_create(tool, 0)) == NULL) {
		fputs("Can't create output_panel\n", stderr);
		exit(1);
	}
	output_panel = output_panel_sw->ts_data;
	output_item = panel_create_item(output_panel, PANEL_MESSAGE,
	    PANEL_LABEL_STRING, "Select with left mouse button.", 0);

	signal(SIGWINCH, sigwinched);
	tool_install(tool);	/* install tool in window tree */
	tool_select(tool, 0);	/* main loop to read input */
	tool_destroy(tool);	/* clean up tool */
}

sigwinched()	/* note window size change and damage repair signal */
{
	tool_sigwinch(tool);
}

date_proc(item, event)	/* get and display date and time */
	Panel_item item;
	struct inputevent *event;
{
	long clock;
	char *ctime();

	time(&clock);
	panel_set(output_item, PANEL_LABEL_STRING, ctime(&clock), 0);
}

rusage_proc(item, event)	/* get and display resource usage */
	Panel_item item;
	struct inputevent *event;
{
	struct rusage rusage;
	static char buf[80];

	getrusage(RUSAGE_SELF, &rusage);
	sprintf(buf, "User %D secs %D millisecs; System %D secs %D millisecs",
	    rusage.ru_utime.tv_sec, rusage.ru_utime.tv_usec/1000,
	    rusage.ru_stime.tv_sec, rusage.ru_stime.tv_usec/1000);
	panel_set(output_item, PANEL_LABEL_STRING, buf, 0);
}

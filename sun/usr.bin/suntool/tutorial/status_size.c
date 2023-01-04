#ifndef lint
static	char sccsid[] = "@(#)status_size.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <suntool/tool_hs.h>
#include <suntool/panel.h>
#include <suntool/gfxsw.h>
#include <sys/resource.h>

struct tool	*tool;
struct toolsw	*control_panel_sw, *gfx_sw;
struct gfxsubwindow *gfx;
Panel		control_panel;
Panel_item	date_item, rusage_item, latest_command;
int		sigwinched(), date_proc(), rusage_proc(), gfx_sigwinch();

unsigned short	icon_image[256] = {
#include <images/status.icon>
};
DEFINE_ICON_FROM_IMAGE(icon, icon_image);

unsigned short clock_image[256] = {	/* clock outline */
#include <images/clocktool.icon>
};
mpr_static(clock_pr, 64, 64, 1, clock_image);
#include "/usr/src/sun/suntool/clockhands.h"	/* Define struct hand */
#include "/usr/src/sun/suntool/clockhands.c"	/* Table of hand positions */

main(argc, argv)	/* panel subwindow for date or resource usage */
	int argc;
	char *argv[];
{

	if ((tool = tool_make(WIN_LABEL,argv[0], WIN_ICON,&icon, 0)) == NULL) {
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

	/* setup graphics subwindow */
	if ((gfx_sw = gfxsw_createtoolsubwindow(tool, "", 
	    TOOL_SWEXTENDTOEDGE, TOOL_SWEXTENDTOEDGE, NULL)) == NULL) {
		fputs("Can't create graphics subwindow\n", stderr);
		exit(1);
	}
	gfx = (struct gfxsubwindow *) gfx_sw->ts_data;
	gfxsw_getretained(gfx);
	gfx_sw->ts_io.tio_handlesigwinch = gfx_sigwinch;

	signal(SIGWINCH, sigwinched);
	tool_install(tool);	/* install tool in window tree */
	tool_select(tool, 0);	/* main loop to read input */
	tool_destroy(tool);	/* clean up tool */
}

sigwinched()	/* note window size change and damage repair signal */
{
	tool_sigwinch(tool);
}

date_proc(item, event)	/* put clock in graphics subwindow */
	Panel_item item;
	struct inputevent *event;
{
#define DATE_X_OFFSET 10
#define DATE_Y_OFFSET 10
	long clock;
	struct tm *local;
	struct hands *hand;

	time(&clock);
	local = localtime(&clock);		/* get time of day */
	/* Initialize the graphics subwindow to grey */
	pw_replrop(gfx->gfx_pixwin, 0, 0, gfx->gfx_rect.r_width,
	    gfx->gfx_rect.r_height, PIX_SRC, tool_bkgrd, 0, 0);
	/*  write clock outline */
	pw_write(gfx->gfx_pixwin, DATE_X_OFFSET, DATE_Y_OFFSET,
	    clock_pr.pr_width, clock_pr.pr_height, PIX_SRC, &clock_pr, 0, 0);
	/* write hour hand */
	hand = &hand_points[(local->tm_hour*5 + (local->tm_min + 6)/12) % 60];
	pw_vector(gfx->gfx_pixwin,
	    DATE_X_OFFSET + hand->x1, DATE_Y_OFFSET + hand->y1,
	    DATE_X_OFFSET + hand->hour_x, DATE_Y_OFFSET + hand->hour_y,
	    PIX_SET, 0);
	pw_vector(gfx->gfx_pixwin,
	    DATE_X_OFFSET + hand->x2, DATE_Y_OFFSET + hand->y2,
	    DATE_X_OFFSET + hand->hour_x, DATE_Y_OFFSET + hand->hour_y,
	    PIX_SET, 0);
	/* write minute hand */
	hand = &hand_points[local->tm_min];
	pw_vector(gfx->gfx_pixwin,
	    DATE_X_OFFSET + hand->x1, DATE_Y_OFFSET + hand->y1,
	    DATE_X_OFFSET + hand->min_x, DATE_Y_OFFSET + hand->min_y,
	    PIX_SET, 0);
	pw_vector(gfx->gfx_pixwin,
	    DATE_X_OFFSET + hand->x2, DATE_Y_OFFSET + hand->y2,
	    DATE_X_OFFSET + hand->min_x, DATE_Y_OFFSET + hand->min_y,
	    PIX_SET, 0);
	/* write second hand */
	hand = &hand_points[local->tm_sec];
	pw_vector(gfx->gfx_pixwin,
	    DATE_X_OFFSET + hand->sec_x, DATE_Y_OFFSET + hand->sec_y,
	    DATE_X_OFFSET + hand->min_x, DATE_Y_OFFSET + hand->min_y,
	    PIX_SET, 0);
	latest_command = item;
}

rusage_proc(item, event) /* put resource usage in graphics subwindow */
	Panel_item item;
	struct inputevent *event;
{
#define RUSAGE_X_OFFSET	10
#define RUSAGE_Y_OFFSET	20
	struct rusage rusage;
	static char buf[80];

	getrusage(RUSAGE_SELF, &rusage);
	sprintf(buf, "User %D secs %D millisecs; System %D secs %D millisecs",
	    rusage.ru_utime.tv_sec, rusage.ru_utime.tv_usec/1000,
	    rusage.ru_stime.tv_sec, rusage.ru_stime.tv_usec/1000);
	/* clear screen */
	pw_writebackground(gfx->gfx_pixwin, 0, 0,
	    gfx->gfx_rect.r_width, gfx->gfx_rect.r_height, PIX_CLR);
	/* write out time resource usage string in reverse video */
	pw_text(gfx->gfx_pixwin, RUSAGE_X_OFFSET, RUSAGE_Y_OFFSET, PIX_NOT(PIX_SRC),
	    NULL, buf);
	latest_command = item;
}

gfx_sigwinch(sw)
	caddr_t sw;
{
	/* Let graphics subwindow notice that sigwinched */
	gfxsw_interpretesigwinch(gfx);
	/* Let graphics subwindow update retained pixwin */
	gfxsw_handlesigwinch(gfx);
	/* See if need to redraw the window due to size change */
	if (gfx->gfx_flags & GFX_RESTART) {
		gfx->gfx_flags &= ~GFX_RESTART;
		if (latest_command == date_item)
			date_proc(date_item, NULL);
		else if (latest_command == rusage_item)
			rusage_proc(rusage_item, NULL);
		/* else already clear */
	}
}


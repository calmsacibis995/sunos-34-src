#ifndef lint
static	char sccsid[] = "@(#)tektool.c 1.3 87/01/07";
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 * Author: Steve Kleiman
 *
 * Overview:	Tek Tool: A tektronix 4014 emulator subwindow (graphics
 *		programs are set up to run in same window).
 */

#include <sys/types.h>
#include <sys/file.h>
#include <signal.h>
#include <stdio.h>

#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_screen.h>
#include <suntool/icon.h>
#include <suntool/tool.h>

#include <suntool/teksw.h>

static	short icon_data[256] = {
#include <images/tektool.icon>
};

mpr_static(tekic_mpr, 64, 64, 1, icon_data);

static	struct icon icon = {64, 64, (struct pixrect *)0, 0, 0, 64, 64,
	    &tekic_mpr, 0, 0, 0, 0, (char *)0, (struct pixfont *)0,
	    ICON_BKGRDGRY};

static	int sigwinchcatcher(), sigchldcatcher();

static	struct tool *tool;

#ifdef STANDALONE
main(argc, argv)
#else
tektool_main(argc, argv)
#endif
int argc;
char **argv;
{
	char **tool_attrs = NULL;
	char *tool_name = argv[0];
	struct toolsw *teksw;
	struct screen screen;
	int pwfd;
	char pwname[WIN_NAMESIZE];

	/*
	 * Get size of screen.
	 */
	if(we_getparentwindow(pwname)){
		fprintf(stderr, "cannot find parent\n");
		exit(1);
	}
	if((pwfd = open(pwname, O_RDONLY, 0)) < 0){
		fprintf(stderr, "cannot open parent window\n");
		exit(1);
	}
	win_screenget(pwfd, &screen);
	close(pwfd);
	argv++;
	argc--;
	/*
	 * Pick up command line arguments to modify tool behavior
	 */
	if (tool_parse_all(&argc, argv, &tool_attrs, tool_name) == -1) {
		tool_usage(tool_name);
		exit(1);
	}
	/*
	 * Create tool window
	 */
	tool = tool_make(
	    WIN_LABEL,		"tektool",
	    WIN_NAME_STRIPE,	1,
	    WIN_ICON,		&icon,
	    WIN_WIDTH,		screen.scr_rect.r_width,
	    WIN_HEIGHT,		screen.scr_rect.r_height,
	    WIN_TOP,		screen.scr_rect.r_top,
	    WIN_LEFT,		screen.scr_rect.r_left,
	    WIN_ATTR_LIST,	tool_attrs,
	    0);
	if (tool == (struct tool *)NULL)
		exit(1);
	tool_free_attribute_list(tool_attrs);
	/*
	 * Create tek subwindow
	 */
	teksw = teksw_createtoolsubwindow(tool, "teksw",
		TOOL_SWEXTENDTOEDGE, TOOL_SWEXTENDTOEDGE,
		argv);
	if(teksw == (struct toolsw *)0){
		exit(1);
	}
	/*
	 * Install tool in tree of windows
	 */
	(void) signal(SIGWINCH, sigwinchcatcher);
	(void) signal(SIGCHLD, sigchldcatcher);
	tool_install(tool);
	/*
	 * Start tek process
	 */
	if (teksw_fork(teksw->ts_data, argv, &teksw->ts_io.tio_inputmask,
	    &teksw->ts_io.tio_outputmask, &teksw->ts_io.tio_exceptmask) == -1) {
		perror("tektool");
		exit(1);
	}
	/*
	 * Handle input
	 */
	tool_select(tool, 1 /* means wait for child process to die*/);
	/*
	 * Cleanup
	 */
	tool_destroy(tool);
	exit(0);
}

static
sigchldcatcher()
{
	tool_sigchld(tool);
}

static
sigwinchcatcher()
{
	tool_sigwinch(tool);
}

#ifndef lint
static	char sccsid[] = "@(#)view_surface.c	1.4	87/01/07 SMI";
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 * 	Overview:	view_surface:   A separate process creates a tool
 *					with an empty subwindow.  The parent
 *					process is a graphics program which
 *					will take over the subwindow for
 *					use as a view surface.
 */

#include <sys/types.h>
#include <signal.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_screen.h>
#include <suntool/icon.h>
#include <suntool/tool.h>
#include <suntool/emptysw.h>

static	short icon_data[256] = {
#include <images/core_eye.icon>
};
mpr_static(coreic_mpr, 64, 64, 1, icon_data);

static	struct icon icon = {64, 64, (struct pixrect *)0, 0, 0, 64, 64,
	    &coreic_mpr, 0, 0, 0, 0, (char *)0, (struct pixfont *)0,
	    ICON_BKGRDGRY};

static	int sigwinchcatcher();

static	struct tool *tool;

static	char *normalname = "view_surface";
#ifdef STANDALONE
main(argc, argv)
#else
view_surface_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	char	*toolname = normalname;
	struct	toolsw *emptysw;
	char emptyswname[SCR_NAMESIZE];

	/*
	 * Create tool window
	 */
	if (argc > 1)
		(void)we_setparentwindow(argv[1]);
	tool = tool_create(toolname, TOOL_NAMESTRIPE, (struct rect *)0, &icon);
	if (tool == (struct tool *)0)
		exit(1);
	/*
	 * Create empty subwindow
	 */
	emptysw = esw_createtoolsubwindow(tool, "emptysw", TOOL_SWEXTENDTOEDGE,
	    TOOL_SWEXTENDTOEDGE);
	if (emptysw == (struct toolsw *)0)
		exit(1);
	/*
	 * Install tool in tree of windows
	 */
	(void)signal(SIGWINCH, sigwinchcatcher);
	(void)tool_install(tool);
	/* 
	 * Send name of subwindow down pipe to parent process and close pipe
	 */
	(void)win_fdtoname(emptysw->ts_windowfd, emptyswname);
	(void)write(1, emptyswname, SCR_NAMESIZE);
	(void)close(1);
	/*
	 * Wait until killed by parent process, handling any window management
	 * functions the user invokes (open, close, stretch, etc.)
	 */
	(void)tool_select(tool, 0);
	/*
	 * Cleanup
	 */
	(void)tool_destroy(tool);
	exit(0);
}

static
sigwinchcatcher()
{
	(void)tool_sigwinch(tool);
}

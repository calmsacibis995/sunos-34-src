#ifndef lint
static	char sccsid[] = "@(#)wmgr_menu.c 1.4 87/01/07 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Window mgr menu handling.
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <strings.h>
#include <suntool/tool_hs.h>
#include <suntool/menu.h>

extern	int (*win_errorhandler())();
extern  void wmgr_changestate(), wmgr_changerect(),wmgr_changelevelonly(),
		wmgr_refreshwindow();
extern	int errno;

#define TOOL_REFRESH	(caddr_t)1
#define TOOL_QUIT	(caddr_t)2
#define TOOL_OPEN	(caddr_t)3
#define TOOL_CLOSE	(caddr_t)4
#define TOOL_MOVE	(caddr_t)5
#define TOOL_STRETCH	(caddr_t)6
#define TOOL_TOP	(caddr_t)7
#define TOOL_BOTTOM	(caddr_t)8

/*
 * Following menu positions are used by tool_input.c.
 */
int	WMGR_STATE_POS = 0;
int	WMGR_MOVE_POS = 1;
int	WMGR_STRETCH_POS = 2;
int	WMGR_TOP_POS = 3;
int	WMGR_BOTTOM_POS = 4;
int	WMGR_REFRESH_POS = 5;
int	WMGR_DESTROY_POS = 6;
struct	menuitem tool_items[] = {
	MENU_IMAGESTRING,	"Open",		TOOL_OPEN,
	MENU_IMAGESTRING,	"Move",		TOOL_MOVE,
	MENU_IMAGESTRING,	"Resize",	TOOL_STRETCH,
	MENU_IMAGESTRING,	"Expose",	TOOL_TOP,
	MENU_IMAGESTRING,	"Hide",		TOOL_BOTTOM,
	MENU_IMAGESTRING,	"Redisplay",	TOOL_REFRESH,
	MENU_IMAGESTRING,	"Quit",		TOOL_QUIT,
};

struct	menu wmgr_toolmenubody = {
	MENU_IMAGESTRING,
	"Frame",
	sizeof(tool_items) / sizeof(struct menuitem),
	tool_items,
	0,
	0
};
struct	menu *wmgr_toolmenu = &wmgr_toolmenubody;

static	int wmgr_error();
static	int (*wmgr_errorcached)();

void
wmgr_open(toolfd, rootfd)
	int	toolfd, rootfd;
{
	void wmgr_set_tool_level();
	
	(void)win_lockdata(toolfd);
	wmgr_changestate(toolfd, rootfd, FALSE);
	wmgr_set_tool_level(rootfd, toolfd);
	(void)win_unlockdata(toolfd);
}

void
wmgr_close(toolfd, rootfd)
	int	toolfd, rootfd;
{
	void wmgr_set_icon_level();
	
	(void)win_lockdata(toolfd);
	wmgr_changestate(toolfd, rootfd, TRUE);
	wmgr_set_icon_level(rootfd, toolfd);
	(void)win_unlockdata(toolfd);
}

void
wmgr_move(toolfd)
	int	toolfd;
{
	struct	inputevent event;

	wmgr_changerect(toolfd, toolfd, &event, TRUE, FALSE);
}

void
wmgr_stretch(toolfd)
	int	toolfd;
{
	struct	inputevent event;

	wmgr_changerect(toolfd, toolfd, &event, FALSE, FALSE);
}

void
wmgr_top(toolfd, rootfd)
	int	toolfd, rootfd;
{
	wmgr_changelevelonly(toolfd, rootfd, TRUE);
}

void
wmgr_bottom(toolfd, rootfd)
	int	toolfd, rootfd;
{	
	wmgr_changelevelonly(toolfd, rootfd, FALSE);
}

/*ARGSUSED*/
wmgr_handletoolmenuitem(menu, mi, toolfd, rootfd)
	struct	menu *menu;
	struct	menuitem *mi;
	int	toolfd, rootfd;
{
	int	returncode = 0;

	wmgr_errorcached = (int (*)())win_errorhandler(wmgr_error);
	switch (mi->mi_data) {
	case TOOL_OPEN:
		wmgr_open(toolfd, rootfd);
		break;
	case TOOL_CLOSE:
		wmgr_close(toolfd, rootfd);
		break;
	case TOOL_MOVE:
		wmgr_move(toolfd);
		break;
	case TOOL_STRETCH:
		wmgr_stretch(toolfd);
		break;
	case TOOL_TOP:
		wmgr_top(toolfd, rootfd);
		break;
	case TOOL_BOTTOM:
		wmgr_bottom(toolfd, rootfd);
		break;
	case TOOL_REFRESH:
		wmgr_refreshwindow(toolfd);
		break;
	case TOOL_QUIT:
		returncode = wmgr_confirm(toolfd,
		    "Press the left mouse button to confirm Quit.  \
To cancel, press the right mouse button now.");
		break;
	}
	(void) win_errorhandler(wmgr_errorcached);
	return(returncode);
}

void
wmgr_setupmenu(windowfd)
	int	windowfd;
{
	if (wmgr_iswindowopen(windowfd)) {
		tool_items[WMGR_STATE_POS].mi_imagedata = "Close";
		tool_items[WMGR_STATE_POS].mi_data = TOOL_CLOSE;
	} else {
		tool_items[WMGR_STATE_POS].mi_imagedata = "Open";
		tool_items[WMGR_STATE_POS].mi_data = TOOL_OPEN;
	}
	return;
}

#define	ARGS_MAX	100

wmgr_forktool(programname, otherargs, rectnormal, recticon, iconic)
	char	*programname, *otherargs;
	struct	rect *rectnormal, *recticon;
	int	iconic;
{
	int	pid;
	char	*args[ARGS_MAX];
 	char	*otherargs_copy;
 	extern	char *calloc();

	(void)we_setinitdata(rectnormal, recticon, iconic);
 	/*
 	 * Copy otherargs because using vfork and don't want to modify
 	 * otherargs that is passed in.
 	 */
 	if (otherargs) {
 		if ((otherargs_copy = calloc(1, (unsigned) (LINT_CAST(
			strlen(otherargs)+1)))) == NULL) {
 			perror("calloc");
 			return(-1);
 		}
 		(void)strcpy(otherargs_copy, otherargs);
 	} else
 		otherargs_copy = NULL;
 	pid = vfork();
	if (pid < 0) {
		perror("fork");
		return(-1);
	}
 	if (pid) {
 		if (otherargs)
 			free(otherargs_copy);
		return(pid);
	}
	/*
	 * Could nice(2) here so that window manager has higher priority
	 * but this also has the affect of making some of the deamons higher
	 * priority.  This can be a problem because when they startup they
	 * preempt the user.
	 */
	/*
	 * Separate otherargs into args
	 */
	(void) constructargs(args, programname, otherargs_copy, ARGS_MAX);
	execvp(programname, args);
	perror(programname);
	exit(1);
	/*NOTREACHED*/
}

int
wmgr_confirm(windowfd, text)
	int	windowfd;
	char	*text;
{
	int	returncode = 0;
	struct	prompt prompt;
	struct	inputevent event;
	extern	struct pixfont *pw_pfsysopen();

	/*
	 * Display a prompt
	 */
	rect_construct(&prompt.prt_rect,
	    PROMPT_FLEXIBLE, PROMPT_FLEXIBLE,
	    PROMPT_FLEXIBLE, PROMPT_FLEXIBLE);
	prompt.prt_font = pw_pfsysopen();
	prompt.prt_text = text;
	(void)menu_prompt(&prompt, &event, windowfd);
	(void)pw_pfsysclose();
	/*
	 * See if user wants to continue operation based on last action
	 */
	if ((event.ie_code==SELECT_BUT) && win_inputposevent(&event))
		returncode = -1;
	return(returncode);
}

int
constructargs(args, programname, otherargs, maxargcount)
	char	*args[], *programname, *otherargs;
	int	maxargcount;
{
#define	terminatearg() {*cpt = NULL;needargstart = 1;}
#define	STRINGQUOTE	'"'
	int	argindex = 0, needargstart = 1, quotedstring = 0;
	register char *cpt;

	args[argindex++] = programname;
	for (cpt = otherargs;(cpt != 0) && (*cpt != NULL);cpt++) {
		if (quotedstring) {
			if (*cpt == STRINGQUOTE) {
				terminatearg();
				quotedstring = 0;
			} else {/* Accept char in arg */}
		} else if (isspace(*cpt)) {
			terminatearg();
		} else {
			if (needargstart && (argindex < maxargcount)) {
				args[argindex++] = cpt;
				needargstart = 0;
			}
			if (*cpt == STRINGQUOTE) {
				/*
				 * Advance cpt in current arg
				 */
				args[argindex-1] = cpt+1;
				quotedstring = 1;
			}
		}
	}
	args[argindex] = '\0';
	return(argindex);
}

/*
 * Error handling
 */
static
wmgr_error(errnum, winopnum)
	int	errnum, winopnum;
{
	switch (errnum) {
	case 0:
		return;
	case -1:
		/*
		 * Probably an ioctl err (could check winopnum)
		 */
		switch (errno) {
		case ENOENT:
		case ENXIO:
		case EBADF:
		case ENODEV:
			break;
			/*
			 * Tool must have gone away
			 * Note: Should do a longjmp to abort the operation
			 */
		default:
			wmgr_errorcached(errnum, winopnum);
			break;
		}
		return;
	default:
		(void)fprintf(stderr, "Window mgr operation %d produced error %d\n",
		    winopnum, errnum);
	}
}

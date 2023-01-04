#ifndef lint
static	char sccsid[] = "@(#)confirm.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sys/file.h>
#include "setup.h"

static Tool		*confirm_tool;
static int		confirm_tool_fd;

static Panel		confirm_panel;
static int		confirm_panel_fd;
static Panel_item	question_item, ok_item, no_item, yes_item;

static int		confirm_response;

static void 		confirm_done();
static void		prepare_links();

void
confirm_init()
{
    confirm_tool = tool_begin(
	WIN_LEFT, 600,
	WIN_TOP, 95,
	WIN_WIDTH, 400,
	WIN_HEIGHT, 60,
	WIN_NAME_STRIPE, FALSE,
	0);
    confirm_tool_fd = confirm_tool->tl_windowfd;

    confirm_panel = panel_begin(confirm_tool,
                                PANEL_FONT, text_font,
                                0);
    confirm_panel_fd = win_get_fd(confirm_panel);

    question_item = 
	panel_create_item(confirm_panel, PANEL_MESSAGE, 0);

    ok_item = 
	panel_create_item(confirm_panel, PANEL_BUTTON,
	    PANEL_ITEM_X, 60,
	    PANEL_ITEM_Y, 20,
	    PANEL_LABEL_IMAGE, image_ok,
	    PANEL_SHOW_ITEM, FALSE,
	    PANEL_NOTIFY_PROC, confirm_done,
	    0);
    no_item = 
	panel_create_item(confirm_panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE, image_no,
	    PANEL_SHOW_ITEM, FALSE,
	    PANEL_NOTIFY_PROC, confirm_done,
	    0);
    yes_item = 
	panel_create_item(confirm_panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE, image_yes,
	    PANEL_SHOW_ITEM, FALSE,
	    PANEL_NOTIFY_PROC, confirm_done,
	    0);
}


confirm_destroy()
{
    if (!confirm_tool)
	return;

    tool_done_with_no_confirm(confirm_tool);
    confirm_tool = 0;
}


int
confirm(question, ok_only)
char	*question;
int	ok_only;
{
    Event	event;
    int		right_down;

    confirm_response = -1;
    panel_set(question_item, PANEL_LABEL_STRING, question, 0);
    if (ok_only)
       panel_set(ok_item, PANEL_SHOW_ITEM, TRUE, 0);
    else {
       panel_set(no_item, PANEL_SHOW_ITEM, TRUE, 0);
       panel_set(yes_item, PANEL_SHOW_ITEM, TRUE, 0);
    }
      
    prepare_links(confirm_tool_fd);
    tool_install(confirm_tool);
    win_post_id(confirm_tool, WIN_REPAINT, NOTIFY_IMMEDIATE);
    win_post_id(confirm_panel, WIN_REPAINT, NOTIFY_IMMEDIATE);
    win_grabio(confirm_panel_fd);
    /* temp hack to not pass through MS_RIGHT down/up/drag */
    right_down = FALSE;
    while (confirm_response == -1) {
	input_readevent(confirm_panel_fd, &event);
	if (event_id(&event) == MS_RIGHT) {
	   right_down = event_is_down(&event);
	   continue;
	}
	if (event_id(&event) != LOC_DRAG || !right_down)
	    win_post_event(confirm_panel, &event, NOTIFY_IMMEDIATE);
    }
    win_releaseio(confirm_panel_fd);
    tool_remove(confirm_tool);

    /* tell the underlying text subwindow to repaint */
    message_repaint();

    panel_set(ok_item, PANEL_SHOW_ITEM, FALSE, 0);
    panel_set(no_item, PANEL_SHOW_ITEM, FALSE, 0);
    panel_set(yes_item, PANEL_SHOW_ITEM, FALSE, 0);

    return confirm_response;
}


int
confirm_yes_no(question)
char	*question;
{
    return confirm(question, FALSE);
}


void
confirm_ok(question)
char	*question;
{
    confirm(question, TRUE);
}


static void
confirm_done(item, event)
Panel_item	item;
Event		*event;
{
    if (item == no_item)
	confirm_response = FALSE;
    else
	confirm_response = TRUE;
}


/* set the links up for a window to
 * be on top.
 */
static void
prepare_links(fd)
int fd;
{
    int 	parent_fd, link;
    char	parentname[WIN_NAMESIZE];

    /*	Determine parent	*/

    if (we_getparentwindow(parentname)) {
	setup_error("Tool not passed parent window in environment\n");
	return;
    }

    /*  Open parent window */

    if ((parent_fd = open(parentname, O_RDONLY, 0)) < 0) {
	setup_error("%s (parent) would not open\n", parentname);
	return;
    }

    /*  Setup links	*/

    link = win_getlink(parent_fd, WL_TOPCHILD);
    win_setlink(fd, WL_OLDERSIB, link);
    win_setlink(fd, WL_YOUNGERSIB, WIN_NULLLINK);

    close(parent_fd);
}

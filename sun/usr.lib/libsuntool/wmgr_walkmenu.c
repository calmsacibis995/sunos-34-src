#ifndef lint
static  char sccsid[] = "@(#)wmgr_walkmenu.c 1.4 87/01/07 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Window mgr walkmenu handling.
 */

#include <stdio.h>
#include <ctype.h>
#include <suntool/tool_hs.h>
#include "tool_impl.h"
#include <suntool/menu.h>
#include <suntool/walkmenu.h>

#define TOOL_MENU_ITEM(label, proc) \
  MENU_ITEM, MENU_STRING, label, MENU_ACTION, proc, MENU_CLIENT_DATA, tool, 0

void wmgr_top(), wmgr_open();
/*
 * Tool menu creation
 */

void		tool_menu_cstretch(), tool_menu_stretch(),
		tool_menu_zoom(), tool_menu_fullscreen(),
		tool_menu_expose(), tool_menu_hide(), tool_menu_open(),
		tool_menu_move(), tool_menu_cmove(), tool_menu_redisplay(),
		tool_menu_quit();
Menu_item	tool_menu_zoom_state(), tool_menu_open_state();
void		wmgr_fullscreen();

extern void 	wmgr_changerect();

     
Menu
tool_walkmenu(tool)
	register Tool *tool;
{   
    Menu	tool_menu;

    tool_menu = menu_create(
	MENU_ITEM,
	  MENU_STRING, "Open",
	  MENU_ACTION, tool_menu_open,
	  MENU_GEN_PROC, tool_menu_open_state,
	  MENU_CLIENT_DATA, tool,
	  0, 
	MENU_PULLRIGHT_ITEM, "Move",
	  menu_create(
		      TOOL_MENU_ITEM("Constrained", tool_menu_cmove),
		      TOOL_MENU_ITEM("Unconstrained", tool_menu_move), 
		      0),
	MENU_PULLRIGHT_ITEM, "Resize",
	  menu_create(
		      TOOL_MENU_ITEM("Constrained", tool_menu_cstretch),
		      TOOL_MENU_ITEM("Unconstrained", tool_menu_stretch),
		      MENU_ITEM,
		        MENU_STRING, "Zoom",
			MENU_ACTION, tool_menu_zoom,
			MENU_GEN_PROC, tool_menu_zoom_state, 
			MENU_CLIENT_DATA, tool,
			0, 
		      TOOL_MENU_ITEM("FullScreen", tool_menu_fullscreen),
		      0),	
	TOOL_MENU_ITEM("Expose", tool_menu_expose),
	TOOL_MENU_ITEM("Hide", tool_menu_hide),
	TOOL_MENU_ITEM("Redisplay", tool_menu_redisplay),
	TOOL_MENU_ITEM("Quit", tool_menu_quit),
	0);

    return tool_menu;
}

/* 
 *  Menu item gen procs
 */
Menu_item
tool_menu_zoom_state(mi, op)
	Menu_item mi;
	Menu_generate op;
{
    Tool *tool;
    
    if (op == MENU_DESTROY) return mi;
    tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    if (tool->tl_flags&TOOL_FULL)
	(void)menu_set(mi, MENU_STRING, "UnZoom", 0);
    else
	(void)menu_set(mi, MENU_STRING, "Zoom", 0);

    return mi;
}

Menu_item
tool_menu_open_state(mi, op)
	Menu_item mi;
	Menu_generate op;
{
    Tool *tool;
    
    if (op == MENU_DESTROY) return mi;
    tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    if (wmgr_iswindowopen(tool->tl_windowfd))
	(void)menu_set(mi, MENU_STRING, "Close", 0);
    else
	(void)menu_set(mi, MENU_STRING, "Open", 0);

    return mi;
}

/*
 *  Callout functions
 */

/*ARGSUSED*/
void    
tool_menu_open(menu, mi)
    Menu	menu;
    Menu_item	mi;
{   
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    int		rootfd = rootfd_for_toolfd(tool->tl_windowfd);
    void	wmgr_close(),wmgr_open();
    
    if (strcmp("Open", menu_get(mi, MENU_STRING)))
	wmgr_close(tool->tl_windowfd, rootfd);
    else
	wmgr_open(tool->tl_windowfd, rootfd);
    (void)close(rootfd);
}

/*ARGSUSED*/
void
tool_menu_move(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    struct	inputevent event;
    
    wmgr_changerect(tool->tl_windowfd, tool->tl_windowfd, &event, TRUE, -1);
}


/*ARGSUSED*/
void
tool_menu_cmove(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    struct	inputevent event;
    
    wmgr_changerect(tool->tl_windowfd, tool->tl_windowfd, &event, TRUE, -2);
}


/*ARGSUSED*/
void
tool_menu_stretch(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    struct	inputevent event;

    wmgr_changerect(tool->tl_windowfd, tool->tl_windowfd, &event, FALSE, -1);
}

/*ARGSUSED*/
void
tool_menu_cstretch(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    struct	inputevent event;

    wmgr_changerect(tool->tl_windowfd, tool->tl_windowfd, &event, FALSE, -2);
}


/*ARGSUSED*/
void
tool_menu_zoom(menu, mi)
    Menu	menu;
    Menu_item	mi;
{   
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    int		rootfd = rootfd_for_toolfd(tool->tl_windowfd);
    void wmgr_full();
             
    wmgr_full(tool, rootfd);
    (void)close(rootfd);
}


/*ARGSUSED*/
void
tool_menu_fullscreen(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    int		rootfd = rootfd_for_toolfd(tool->tl_windowfd);
             
    wmgr_fullscreen(tool, rootfd);
    (void)close(rootfd);
}


/*ARGSUSED*/
void
tool_menu_expose(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    int		rootfd = rootfd_for_toolfd(tool->tl_windowfd);
    
    wmgr_top(tool->tl_windowfd, rootfd);
    (void)close(rootfd);
}


/*ARGSUSED*/
void
tool_menu_hide(menu, mi)
    Menu	menu;
    Menu_item	mi;
{   
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    int		rootfd = rootfd_for_toolfd(tool->tl_windowfd);
    void	wmgr_bottom();
    
    wmgr_bottom(tool->tl_windowfd, rootfd);
    (void)close(rootfd);
}


/*ARGSUSED*/
void
tool_menu_redisplay(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));
    void wmgr_refreshwindow();
    
    wmgr_refreshwindow(tool->tl_windowfd);
}


/*ARGSUSED*/
void
tool_menu_quit(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Tool	*tool = (Tool *)(LINT_CAST(menu_get(mi, MENU_CLIENT_DATA)));

   /*
    * Handle "Quit" differently in notifier world.
    * Will confirm quit only if no other entity vetos or confirms.
    */
    if ((tool->tl_flags & TOOL_NOTIFIER)) {
	(void)tool_done(tool);
    } else if (wmgr_confirm(tool->tl_windowfd,
"Press the left mouse button to confirm Quit.  To cancel, press the right mouse button now."))
        (void)tool_done_with_no_confirm(tool);
}


void
wmgr_fullscreen(tool, rootfd)

	Tool 	*tool; 
	int	rootfd;
{
	Rect	oldopenrect, fullrect;
	int	iconic = tool_is_iconic(tool);
			
	if (iconic) (void)win_getsavedrect(tool->tl_windowfd, &oldopenrect);
	else (void)win_getrect(tool->tl_windowfd, &oldopenrect);
	(void)win_getrect(rootfd, &fullrect);
	if (rect_equal(&fullrect, &oldopenrect)) return; /* punt */
	(void)win_lockdata(tool->tl_windowfd);
	if (!(tool->tl_flags&TOOL_FULL)) tool->tl_openrect = oldopenrect;
	tool->tl_flags |= TOOL_FULL;
	if (iconic) {
	    (void)win_setsavedrect(tool->tl_windowfd, &fullrect);
	    (void)tool_layoutsubwindows(tool);
	    wmgr_open(tool->tl_windowfd,rootfd);
	} else {
	    wmgr_top(tool->tl_windowfd,rootfd);		
	    (void)win_setrect(tool->tl_windowfd, &fullrect);
	    (void)tool_layoutsubwindows(tool);
	}		
	(void)win_unlockdata(tool->tl_windowfd);
}

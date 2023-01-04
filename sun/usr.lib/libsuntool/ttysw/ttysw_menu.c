#ifndef lint
static  char sccsid[] = "@(#)ttysw_menu.c 1.9 87/04/20 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ttysw menu initialization and call-back procedures
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>

#include <stdio.h>
#include <ctype.h>

#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/win_input.h>
#include <suntool/ttysw.h>
#include <suntool/textsw.h>
#include <suntool/walkmenu.h>
#include <suntool/menu.h>
#include <suntool/selection.h>
#include <suntool/selection_svc.h>
#include "ttysw_impl.h"

extern void ttygetselection();
extern void ttysw_do_put_get();
extern struct pixrect *mem_create();
extern Textsw_status	textsw_set();

/* generally applicable ttysw menu definitions */

static char ttysw_put_then_get[] = "Put, then Get";
static char ttysw_get_only[]     = "          Get";
static char ttysw_put_then[] =     "Put, then";
static char ttysw_scrolling_enable[]	= "Enable Scroll";


/* ttysw old non-walking menu definitions */

#define TTYSW_STUFF	 (caddr_t)1
#define TTYSW_PAGE	 (caddr_t)2
#define TTYSW_HIST	 (caddr_t)3
#define TTYSW_FLUSH	 (caddr_t)4
#define TTYSW_PUT_GET	 (caddr_t)5
#define TTYSW_ENABLE_SCROLLING	 (caddr_t)6

static struct menuitem ttysw_menuitems[] = {
	MENU_IMAGESTRING, "Stuff", TTYSW_STUFF,
	MENU_IMAGESTRING, 0, TTYSW_PAGE,
#ifdef TTYHIST
	MENU_IMAGESTRING, 0, TTYSW_HIST,
#endif
	MENU_IMAGESTRING, 0, TTYSW_PUT_GET,
	MENU_IMAGESTRING, "Flush Input", TTYSW_FLUSH,
	MENU_IMAGESTRING, ttysw_scrolling_enable, TTYSW_ENABLE_SCROLLING
};
static struct menuitem ttysw_put_get_item = {
	MENU_IMAGESTRING, 0, TTYSW_PUT_GET
};

static struct menuitem ttysw_flush_item = {
	MENU_IMAGESTRING, "Flush Input", TTYSW_FLUSH
};

static struct menuitem ttysw_enable_scrolling_item = {
	MENU_IMAGESTRING, ttysw_scrolling_enable, TTYSW_ENABLE_SCROLLING
};

static struct menu  ttysw_menubody = {
	MENU_IMAGESTRING,
	"tty",
	sizeof (ttysw_menuitems) / sizeof (struct menuitem),
	ttysw_menuitems,
	0,
	0
};

/* shorthand */
#define	iwbp	ttysw->ttysw_ibuf.cb_wbp
#define	irbp	ttysw->ttysw_ibuf.cb_rbp

/* ttysw non-walking menu definitions */


/* ttysw walking menu definitions */

static Menu_item	 ttysw_menu_page_state();
static Menu_item	 ttysw_menu_put_get_state();
static void		 ttysw_show_walkmenu();

static void		 ttysw_menu_stuff();
static void		 ttysw_enable_scrolling();
static void		 ttysw_disable_scrolling();
static void		 ttysw_menu_page();
static void		 ttysw_menu_flush();
static void		 ttysw_menu_history();
static void		 ttysw_menu_put_get();

static struct pixrect	*ttysw_get_only_pr;

/* cmdsw walking menu definitions */

extern Cmdsw	*cmdsw;

typedef enum {
	TTYSW_APPEND_ONLY		= TEXTSW_MENU_LAST_CMD + 1,
	TTYSW_LAST_CMD			= TEXTSW_MENU_LAST_CMD + 2
}	Ttysw_menu_cmd;

static char	ttysw_menu_edit_on[]	= "Enable Edit";
static char	ttysw_menu_edit_off[]	= "Disable Edit";
char		ttysw_scrolling_disable[]	= "Disable Scroll";

/* ttysw old non-walking menu utilities */

int
ttysw_process_menu(ttysw, ie)
    register struct ttysubwindow *ttysw;
    register struct inputevent *ie;
{
    extern struct menuitem *menu_display();
    struct menuitem    *mi;
    struct menu        *menuptr = &ttysw_menubody;
    int                 wfd = ttysw->ttysw_wfd;
    int                 show_flush, show_put_get, pagemode;

    if (win_inputnegevent(ie)) {
	return TTY_OK;
    }

    show_flush = (iwbp > irbp);
    show_put_get = ttysw_getopt((caddr_t) ttysw, TTYOPT_SELSVC);

    /* use the walking menu if present */
    if (ttysw->ttysw_menu) {
	ttysw_show_walkmenu(ttysw, ie, show_flush);
	return (TTY_DONE);
    }

    if (ie->ie_shiftmask & SHIFTMASK) {
	ttygetselection(ttysw);
	return (TTY_DONE);
    }
    pagemode = ttysw_getopt((caddr_t) ttysw, TTYOPT_PAGEMODE);
    /* Change the constant 3, when number of items is changed in ttysw_menuitems */
    menuptr->m_itemcount = (sizeof (ttysw_menuitems) /
			    sizeof (struct menuitem)) - 3;
    if (show_put_get) {
	if (ttysw_is_seln_nonzero(ttysw, SELN_PRIMARY)) {
	    ttysw_menuitems[menuptr->m_itemcount] = ttysw_put_get_item;
	    ttysw_menuitems[menuptr->m_itemcount].mi_imagedata =
	    		(caddr_t) ttysw_put_then_get;
	    menuptr->m_itemcount++;
	} else if (ttysw_is_seln_nonzero(ttysw, SELN_SHELF)) {
	    ttysw_menuitems[menuptr->m_itemcount] = ttysw_put_get_item;
	    ttysw_menuitems[menuptr->m_itemcount].mi_imagedata =
	    		(caddr_t) ttysw_get_only;
	    menuptr->m_itemcount++;
	}
    }
    if (show_flush) {
	ttysw_menuitems[menuptr->m_itemcount] = ttysw_flush_item;
	menuptr->m_itemcount++;
    }
    
    if (ttysw->ttysw_hist) {
    	ttysw_menuitems[menuptr->m_itemcount] = ttysw_enable_scrolling_item;
	menuptr->m_itemcount++;
    }
    
    if (ttysw->ttysw_frozen)
	ttysw_menuitems[1].mi_imagedata = (caddr_t) "Continue     ";
    else if (pagemode)
	ttysw_menuitems[1].mi_imagedata = (caddr_t) "Disable Page Mode";
    else
	ttysw_menuitems[1].mi_imagedata = (caddr_t) "Enable Page Mode ";
#ifdef TTYHIST
    if (history)
	ttysw_menuitems[2].mi_imagedata = (caddr_t) "Disable History";
    else
	ttysw_menuitems[2].mi_imagedata = (caddr_t) "Enable History ";
#endif
    if ((mi = menu_display(&menuptr, ie, wfd)) == 0)
	return (TTY_DONE);
    switch (mi->mi_data) {
      case TTYSW_STUFF:	   /* Get the selection  */
	ttygetselection(ttysw);
	break;
      case TTYSW_PAGE:
	if (ttysw->ttysw_frozen)
	    (void) ttysw_freeze(ttysw, 0);
	else
	    (void)ttysw_setopt((caddr_t) ttysw, TTYOPT_PAGEMODE, !pagemode);
	break;
      case TTYSW_FLUSH:
	(void)ttysw_flush_input((caddr_t) ttysw);
	break;
#ifdef TTYHIST
      case TTYSW_HIST:
	(void)ttysw_setopt((caddr_t) ttysw, TTYOPT_HISTORY, !history);
	break;
#endif
      case TTYSW_PUT_GET:
	ttysw_do_put_get(ttysw);
	break;
      case TTYSW_ENABLE_SCROLLING:
        ttysw_setopt(ttysw, TTYOPT_TEXT, 1);
    }
    return TTY_DONE;

}


/* ttysw walking menu utilities */

Menu
ttysw_walkmenu(ttysw)
	register Ttysw *ttysw;
{   
    Menu	ttysw_menu;
    Menu_item	put_get_item,enable_scrolling_item;


    ttysw_menu = menu_create(
	MENU_ITEM,
	  MENU_STRING, "Stuff",
	  MENU_ACTION, ttysw_menu_stuff,
	  MENU_CLIENT_DATA, ttysw,
	  0, 
	MENU_ITEM,
	  MENU_STRING, "Disable Page Mode",
	  MENU_ACTION, ttysw_menu_page,
	  MENU_GEN_PROC, ttysw_menu_page_state,
	  MENU_CLIENT_DATA, ttysw,
	  0,
	0);
    if (ttysw_get_only_pr == 0) {
	struct pixfont		*menu_font;
	struct pr_subregion	 get_only_bound;
	struct pr_prpos		 get_only_prpos;
	int			 menu_left_margin;
	
	/* compute the size of the pixrect and allocate it */
	menu_font = (struct pixfont *)LINT_CAST(menu_get(ttysw_menu, MENU_FONT));
	(void)pf_textbound(&get_only_bound,
	    strlen(ttysw_put_then_get), menu_font, ttysw_put_then_get);
	menu_left_margin = (int)menu_get(ttysw_menu, MENU_LEFT_MARGIN);
	ttysw_get_only_pr = mem_create(
	    get_only_bound.size.x + menu_left_margin +
	    (int)menu_get(ttysw_menu, MENU_RIGHT_MARGIN),
	    get_only_bound.size.y, 1);
	/* write the text into the pixrect */
	get_only_prpos.pr = ttysw_get_only_pr;
	get_only_prpos.pos.x = menu_left_margin
	    - menu_font->pf_char[ttysw_put_then_get[0]].pc_home.x;
	get_only_prpos.pos.y =
	    - menu_font->pf_char[ttysw_put_then_get[0]].pc_home.y;
	(void)pf_text(get_only_prpos, PIX_SRC, menu_font, ttysw_put_then_get);
	/* gray out "Put, then" */
	(void)pf_textbound(&get_only_bound,
	    strlen(ttysw_put_then), menu_font, ttysw_put_then);
	(void)pr_replrop(ttysw_get_only_pr, menu_left_margin, 0,
		   get_only_bound.size.x, get_only_bound.size.y, 
		   PIX_SRC & PIX_DST, &menu_gray50_pr, 0, 0);
    }
    put_get_item = menu_create_item(
	MENU_STRING, ttysw_put_then_get,
	MENU_ACTION, ttysw_menu_put_get,
	MENU_GEN_PROC, ttysw_menu_put_get_state,
	MENU_CLIENT_DATA, ttysw,
	0); 
    (void)menu_set(ttysw_menu, MENU_APPEND_ITEM, put_get_item, 0);
    
    if (ttysw->ttysw_hist) {
    	enable_scrolling_item = menu_create_item(
		MENU_STRING, ttysw_scrolling_enable,
		MENU_ACTION, ttysw_enable_scrolling,
		MENU_CLIENT_DATA, ttysw,
		MENU_INACTIVE,	TRUE,
		0); 
    	(void)menu_set(ttysw_menu, MENU_APPEND_ITEM, enable_scrolling_item, 0);
    };

    return ttysw_menu;
}


static void
ttysw_show_walkmenu(ttysw, event, show_flush)
	Ttysw	*ttysw;
	Event	*event;
	int	show_flush;
{
    Menu_item	flush_item;

    if (show_flush) {
	flush_item = menu_create_item(
	    MENU_STRING, "Flush",
	    MENU_ACTION, ttysw_menu_flush,
	    MENU_CLIENT_DATA, ttysw,
	    0); 
	(void)menu_set(ttysw->ttysw_menu, MENU_APPEND_ITEM, flush_item, 0);
    }

    (void)menu_show_using_fd(ttysw->ttysw_menu, ttysw->ttysw_wfd, event);

    if (show_flush) {
	(void)menu_set(ttysw->ttysw_menu, MENU_REMOVE_ITEM, flush_item, 0);
	menu_destroy(flush_item);
    }
}


/* 
 *  Menu item gen procs
 */
static Menu_item
ttysw_menu_page_state(mi, op)
	Menu_item mi;
	Menu_generate op;
{
    Ttysw *ttysw;
    
    if (op == MENU_DESTROY)
	return mi;

    ttysw = (Ttysw *) LINT_CAST(menu_get(mi, MENU_CLIENT_DATA));

    if (ttysw->ttysw_frozen)
	(void)menu_set(mi, MENU_STRING, "Continue", 0);
    else if (ttysw_getopt((caddr_t) ttysw, TTYOPT_PAGEMODE))
	(void)menu_set(mi, MENU_STRING, "Disable Page Mode", 0);
    else
	(void)menu_set(mi, MENU_STRING, "Enable Page Mode ", 0);

    return mi;
}

static Menu_item
ttysw_menu_put_get_state(mi, op)
	Menu_item mi;
	Menu_generate op;
{
    Ttysw *ttysw;
    
    if (op == MENU_DESTROY)
	return mi;

    ttysw = (Ttysw *) LINT_CAST(menu_get(mi, MENU_CLIENT_DATA));

    if (ttysw_is_seln_nonzero(ttysw, SELN_PRIMARY)) {
        (void)menu_set(mi,
	    MENU_IMAGE,		0,
	    MENU_STRING,	ttysw_put_then_get,
	    MENU_INACTIVE,	FALSE,
	    0);
    } else if (ttysw_is_seln_nonzero(ttysw, SELN_SHELF)) {
        (void)menu_set(mi,
	    MENU_STRING,	0,
	    MENU_IMAGE,		ttysw_get_only_pr,
	    MENU_INACTIVE,	FALSE,
	    0);
    } else {
        (void)menu_set(mi,
	    MENU_IMAGE,		0,
	    MENU_STRING,	ttysw_put_then_get,
	    MENU_INACTIVE,	TRUE,
	    0);
    }

    return mi;
}

/*
 *  Callout functions
 */



/* ARGSUSED */
static void
ttysw_menu_stuff(menu, mi)
    Menu	menu;
    Menu_item	mi;
{   
    Ttysw	*ttysw = (Ttysw *) LINT_CAST(menu_get(mi, MENU_CLIENT_DATA));
    
    ttygetselection(ttysw);
}

/* ARGSUSED */
static void
ttysw_enable_scrolling(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Ttysw	*ttysw = (Ttysw *) LINT_CAST(menu_get(mi, MENU_CLIENT_DATA));
    ttysw_setopt(ttysw, TTYOPT_TEXT, 1);
}

extern Menu_item
ttysw_get_scroll_cmd_from_menu_for_ttysw(ttysw_menu)
    Menu	ttysw_menu;
{
    /* The reason for using menu_get() is menu_find() has problem. */
    return (Menu_item)menu_get(
	ttysw_menu,
	MENU_NTH_ITEM,
	menu_get(ttysw_menu, MENU_NITEMS, 0));
}

extern Menu_item
ttysw_get_scroll_cmd_from_menu_for_textsw(textsw_menu)
    Menu	textsw_menu;
{
    /* The reason for using menu_get() is menu_find() has problem. */
    
   return (Menu_item)menu_get(
	textsw_menu,
	MENU_NTH_ITEM,
	menu_get(textsw_menu, MENU_NITEMS, 0));
}

extern void
ttysw_set_scrolling_cmds(disable_scroll, enable_scroll, allow_enable)
     Menu_item	disable_scroll, enable_scroll;
     int	allow_enable;
{
    if (allow_enable) {
	(void)menu_set(enable_scroll, MENU_INACTIVE, FALSE, 0); 
	(void)menu_set(disable_scroll, MENU_INACTIVE, TRUE, 0);
    } else {
	(void)menu_set(enable_scroll, MENU_INACTIVE, TRUE, 0); 
	(void)menu_set(disable_scroll, MENU_INACTIVE, FALSE, 0);
    }

}

extern void
ttysw_set_scrolling(textsw_menu, ttysw_menu, allow_enable)
   Menu		textsw_menu, ttysw_menu;
   int		allow_enable;
{
    Menu_item	enable_scroll = ttysw_get_scroll_cmd_from_menu_for_ttysw(
					ttysw_menu);
    Menu_item	disable_scroll = ttysw_get_scroll_cmd_from_menu_for_textsw(
					textsw_menu);

    (void)ttysw_set_scrolling_cmds(
	disable_scroll, enable_scroll, allow_enable); 
}

static void
ttysw_do_enable_scrolling(ttysw)
    Ttysw	*ttysw;
{   
    
    Textsw	textsw = (Textsw)ttysw->ttysw_hist;
    Menu	textsw_menu = (Menu)textsw_get(textsw, TEXTSW_MENU);
    /* The reason for using menu_get() is menu_find() has problem. */

    ttysw_setopt(ttysw, TTYOPT_TEXT, 1);
}

/* ARGSUSED */
static void
ttysw_menu_page(menu, mi)
    Menu	menu;
    Menu_item	mi;
{   
    Ttysw	*ttysw = (Ttysw *) LINT_CAST(menu_get(mi, MENU_CLIENT_DATA));
    
    if (ttysw->ttysw_frozen)
	(void) ttysw_freeze(ttysw, 0);
    else
	(void)ttysw_setopt((caddr_t) ttysw, TTYOPT_PAGEMODE, 
	    !ttysw_getopt((caddr_t) ttysw, TTYOPT_PAGEMODE));
}

/* ARGSUSED */
static void
ttysw_menu_flush(menu, mi)
    Menu	menu;
    Menu_item	mi;
{   
    caddr_t	ttysw = (caddr_t) LINT_CAST(menu_get(mi, MENU_CLIENT_DATA));
    
    (void)ttysw_flush_input(ttysw);
}

/* ARGSUSED */
static void
ttysw_menu_put_get(menu, mi)
    Menu	menu;
    Menu_item	mi;
{   
    Ttysw	*ttysw = (Ttysw *) LINT_CAST(menu_get(mi, MENU_CLIENT_DATA));
    
    ttysw_do_put_get(ttysw);
}

/* cmdsw walking menu definitions */

ttysw_set_menu(textsw)
    Textsw		  textsw;
{
    Menu	 cmd_menu;
    Menu_item	 cmd_item1, cmd_item2;
    int		 ttysw_menu_append_only();

    cmd_menu = (Menu)textsw_get(textsw, TEXTSW_MENU);
    cmd_item1 = (Menu_item)menu_create_item(
    			MENU_VALUE,		TTYSW_APPEND_ONLY,
    			MENU_ACTION,		ttysw_menu_append_only,
    			MENU_CLIENT_DATA,	textsw,
    			MENU_INACTIVE,		FALSE,
    			0);
    cmd_item2 = (Menu_item)menu_create_item(
    			MENU_STRING,		ttysw_scrolling_disable,
    			MENU_ACTION,		ttysw_disable_scrolling,
			MENU_CLIENT_DATA,	textsw,
    			MENU_INACTIVE,		FALSE,
    			0);			
    if (cmdsw->append_only_log) {
	(void)menu_set(cmd_item1,	MENU_STRING,	ttysw_menu_edit_on, 0);
    } else {
	(void)menu_set(cmd_item1,	MENU_STRING,	ttysw_menu_edit_off, 0);
    }
    /*
     * cmd_item2 has to be the last item of the menu; otherwise,
     * it will confuse menu_get() in ttysw_enable_scrolling() and
     * ttysw_text_event().
      
     */
    (void)menu_set(cmd_menu, MENU_APPEND_ITEM, cmd_item1,
                             MENU_APPEND_ITEM, cmd_item2, 0);
}

/* ARGSUSED */
ttysw_menu_append_only(cmd_menu, cmd_item)
    Menu	 cmd_menu;
    Menu_item	 cmd_item;
{
    Textsw	 textsw = (Textsw)(LINT_CAST(menu_get(cmd_item,
    					MENU_CLIENT_DATA)));
    Textsw_index tmp_index, insert;

    cmdsw->append_only_log = !cmdsw->append_only_log;
    if (cmdsw->append_only_log) {
	(void)menu_set(cmd_item,	MENU_STRING,	ttysw_menu_edit_on, 0);
	tmp_index = (int)textsw_find_mark(textsw, cmdsw->pty_mark);
	insert = (Textsw_index)textsw_get(textsw, TEXTSW_INSERTION_POINT);
	if (insert != tmp_index) {
	    (void)textsw_set(textsw, TEXTSW_INSERTION_POINT, tmp_index, 0);
	}
	cmdsw->read_only_mark =
	    textsw_add_mark(textsw,
	        cmdsw->cooked_echo ? tmp_index : TEXTSW_INFINITY-1,
	        TEXTSW_MARK_READ_ONLY);
    } else {
	(void)menu_set(cmd_item,	MENU_STRING,	ttysw_menu_edit_off, 0);
	}
	
   textsw_remove_mark(textsw, cmdsw->read_only_mark);
}


/* ARGSUSED */
static void
ttysw_disable_scrolling(cmd_menu, cmd_menu_item)
    Menu	 cmd_menu;
    Menu_item	 cmd_menu_item;
{
    
    Textsw   	textsw = (Textsw) LINT_CAST (menu_get(cmd_menu_item,
    					  MENU_CLIENT_DATA));
    Ttysw	*ttysw = (Ttysw *) LINT_CAST (textsw_get(textsw,
    					  TEXTSW_CLIENT_DATA));
    Menu	ttysw_menu = (Menu)ttysw->ttysw_menu;
    Menu_item	enable_scroll = ttysw_get_scroll_cmd_from_menu_for_ttysw(
		 			  ttysw_menu);
    int		allow_enable = 1;	 			  					  
    ttysw->ttysw_hist = (FILE *) LINT_CAST(textsw);
    ttysw_setopt(ttysw, TTYOPT_TEXT, 0);
    (void)ttysw_set_scrolling_cmds(cmd_menu_item, enable_scroll, allow_enable); 
    
     	   					
}					

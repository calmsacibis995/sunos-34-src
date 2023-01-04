#ifndef lint
static  char sccsid[] = "@(#)textsw_menu.c 1.22 87/02/24";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *  Text subwindow menu creation and support.
 */

#include "primal.h"

#include "textsw_impl.h"
#include <errno.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_struct.h>
#include <suntool/fullscreen.h>
#include <suntool/menu.h>		/* Contains prompt declarations */
#include <suntool/walkmenu.h>
#include <suntool/window.h>

extern Ev_handle	ev_resolve_xy_to_view();
extern Ev_handle	ev_nearest_view();
extern Es_index		ev_index_for_line();

static char create_view_prompt[] =
"Click the left button where the new view should begin.  \
Abort by clicking the right button.";

static char reset_prompt[] =
"The text has been edited; click the left button to confirm the Reset.  \
Abort by clicking the right button.";

static char load_prompt[] =
"The text has been edited; click the left button to confirm the Load.  \
Abort by clicking the right button.";

static char put_then_get[] = "Put then Get";
static char put_then[]     = "Put then";

static struct pixrect *textsw_get_only_pr;

static int textsw_menu_refcount;	/* refcount for textsw_menu */
static Menu textsw_menu;		/* Let default to NULL */
static Menu_item *textsw_menu_items	/* [TEXTSW_MENU_LAST_CMD] */;

extern int	textsw_has_been_modified();
extern void	tool_expand_neighbors();
pkg_private void      textsw_get_extend_to_edge();
pkg_private void      textsw_set_extend_to_edge();

/* VARARGS0 */
static Menu
textsw_new_menu()
{
    Menu top_menu, save_file, break_mode, find_pattern;
    
    save_file = menu_create(
	MENU_LEFT_MARGIN, 6,
	MENU_STRING_ITEM, "Save file",		TEXTSW_MENU_SAVE,
	MENU_STRING_ITEM, "Save & Quit",	TEXTSW_MENU_SAVE_QUIT,
	MENU_STRING_ITEM, "Save & Reset",	TEXTSW_MENU_SAVE_RESET,
	MENU_STRING_ITEM, "Close & Save", 	TEXTSW_MENU_SAVE_CLOSE, 
	MENU_STRING_ITEM, "Store to named file",TEXTSW_MENU_STORE,
	MENU_STRING_ITEM, "Store & Quit",	TEXTSW_MENU_STORE_QUIT,
	MENU_STRING_ITEM, "Close & Store", 	TEXTSW_MENU_STORE_CLOSE, 
	0);
	
    find_pattern = menu_create(
	MENU_LEFT_MARGIN, 6,
	MENU_STRING_ITEM, "Find, forward",	TEXTSW_MENU_FIND,
	MENU_STRING_ITEM, "Find, backward",	TEXTSW_MENU_FIND_BACKWARD,
	MENU_STRING_ITEM, "Find Shelf, forward",TEXTSW_MENU_FIND_SHELF,
	MENU_STRING_ITEM, "Find Shelf, backward",
					TEXTSW_MENU_FIND_SHELF_BACKWARD,
#ifdef _TEXTSW_FIND_RE
	MENU_STRING_ITEM, "Find RE, forward",	TEXTSW_MENU_FIND_RE,
	MENU_STRING_ITEM, "Find RE, backward",	TEXTSW_MENU_FIND_RE_BACKWARD,
	MENU_STRING_ITEM, "Find tag, forward",	TEXTSW_MENU_FIND_TAG,
	MENU_STRING_ITEM, "Find tag, backward", TEXTSW_MENU_FIND_TAG_BACKWARD, 
#endif
	0);
	
    break_mode = menu_create(
	MENU_LEFT_MARGIN, 6,
	MENU_STRING_ITEM, "Clip lines",        TEXTSW_MENU_CLIP_LINES,
	MENU_STRING_ITEM, "Wrap at character", TEXTSW_MENU_WRAP_LINES_AT_CHAR,
/*	MENU_STRING_ITEM, "Wrap at word",      TEXTSW_MENU_WRAP_LINES_AT_WORD,
*/
	0);
			    
    top_menu = menu_create(
	MENU_LEFT_MARGIN, 6,
	MENU_PULLRIGHT_ITEM, "Save",	      save_file,
	MENU_STRING_ITEM,    "Load file",     TEXTSW_MENU_LOAD,
	MENU_STRING_ITEM,    "Select line #", TEXTSW_MENU_NORMALIZE_LINE,
	MENU_STRING_ITEM,    "Split view",    TEXTSW_MENU_CREATE_VIEW,
	MENU_STRING_ITEM,    "Destroy view",  TEXTSW_MENU_DESTROY_VIEW,
	MENU_STRING_ITEM,    "Reset",	      TEXTSW_MENU_RESET,
	MENU_STRING_ITEM,    "What line #?",  TEXTSW_MENU_COUNT_TO_LINE,
	MENU_STRING_ITEM,    "Get from file", TEXTSW_MENU_FILE_STUFF,
	MENU_STRING_ITEM,    "Caret to top",  TEXTSW_MENU_NORMALIZE_INSERTION,
#ifdef notdef
	MENU_STRING_ITEM,    "Selection to top",
					      TEXTSW_MENU_NORMALIZE_SELECTION,
#endif
	MENU_PULLRIGHT_ITEM, "Line break",    break_mode, 
	MENU_STRING_ITEM,    "Set directory", TEXTSW_MENU_CD,
	MENU_PULLRIGHT_ITEM, "Find",	      find_pattern,
	MENU_STRING_ITEM,    put_then_get,    TEXTSW_MENU_PUT_THEN_GET,
	0);
    return top_menu;
}

extern Menu_item *
textsw_new_menu_table(textsw)
    Textsw_folio textsw;
{
    int textsw_do_menu_action();
    int item;
    Menu_item menu_item, *menu_items;
    Menu top_menu = textsw->menu, save_file, break_mode, find_pattern;

    menu_items = (Menu_item *)LINT_CAST(calloc((unsigned)TEXTSW_MENU_LAST_CMD,
	sizeof(Menu_item)));
    menu_items[(int)TEXTSW_MENU_LOAD] =
	menu_get(top_menu, MENU_NTH_ITEM, 2);
    menu_items[(int)TEXTSW_MENU_NORMALIZE_LINE] =
	menu_get(top_menu, MENU_NTH_ITEM, 3);
    menu_items[(int)TEXTSW_MENU_CREATE_VIEW] =
	menu_get(top_menu, MENU_NTH_ITEM, 4);
    menu_items[(int)TEXTSW_MENU_DESTROY_VIEW] =
	menu_get(top_menu, MENU_NTH_ITEM, 5);
    menu_items[(int)TEXTSW_MENU_RESET] =
	menu_get(top_menu, MENU_NTH_ITEM, 6);
    menu_items[(int)TEXTSW_MENU_COUNT_TO_LINE] =
	menu_get(top_menu, MENU_NTH_ITEM, 7);
    menu_items[(int)TEXTSW_MENU_FILE_STUFF] =
	menu_get(top_menu, MENU_NTH_ITEM, 8);
    menu_items[(int)TEXTSW_MENU_NORMALIZE_INSERTION] =
	menu_get(top_menu, MENU_NTH_ITEM, 9);
    menu_items[(int)TEXTSW_MENU_CD] =
	menu_get(top_menu, MENU_NTH_ITEM, 11);
    menu_items[(int)TEXTSW_MENU_PUT_THEN_GET] =
	menu_get(top_menu, MENU_NTH_ITEM, 13);

    menu_item = (Menu_item)menu_get(top_menu, MENU_NTH_ITEM, 1);
    save_file = (Menu)menu_get(menu_item, MENU_VALUE);
    menu_item = (Menu_item)menu_get(top_menu, MENU_NTH_ITEM, 10);
    break_mode = (Menu)menu_get(menu_item, MENU_VALUE);
    menu_item = (Menu_item)menu_get(top_menu, MENU_NTH_ITEM, 12);
    find_pattern = (Menu)menu_get(menu_item, MENU_VALUE);
    
    menu_items[(int)TEXTSW_MENU_SAVE] =
	menu_get(save_file, MENU_NTH_ITEM, 1);
    menu_items[(int)TEXTSW_MENU_SAVE_QUIT] =
	menu_get(save_file, MENU_NTH_ITEM, 2);
    menu_items[(int)TEXTSW_MENU_SAVE_RESET] =
	menu_get(save_file, MENU_NTH_ITEM, 3);
    menu_items[(int)TEXTSW_MENU_SAVE_CLOSE] =
	menu_get(save_file, MENU_NTH_ITEM, 4); 
    menu_items[(int)TEXTSW_MENU_STORE] =
	menu_get(save_file, MENU_NTH_ITEM, 5);
    menu_items[(int)TEXTSW_MENU_STORE_QUIT] =
	menu_get(save_file, MENU_NTH_ITEM, 6);
    menu_items[(int)TEXTSW_MENU_STORE_CLOSE] =
	menu_get(save_file, MENU_NTH_ITEM, 7); 
	
    menu_items[(int)TEXTSW_MENU_CLIP_LINES] =
	menu_get(break_mode, MENU_NTH_ITEM, 1);
    menu_items[(int)TEXTSW_MENU_WRAP_LINES_AT_CHAR] =
	menu_get(break_mode, MENU_NTH_ITEM, 2);
/* 
    menu_items[(int)TEXTSW_MENU_WRAP_LINES_AT_WORD] =
	menu_get(break_mode, MENU_NTH_ITEM, 3);
*/

    menu_items[(int)TEXTSW_MENU_FIND] =
	menu_get(find_pattern, MENU_NTH_ITEM, 1);
    menu_items[(int)TEXTSW_MENU_FIND_BACKWARD] =
	menu_get(find_pattern, MENU_NTH_ITEM, 2);
    menu_items[(int)TEXTSW_MENU_FIND_SHELF] =
	menu_get(find_pattern, MENU_NTH_ITEM, 3);
    menu_items[(int)TEXTSW_MENU_FIND_SHELF_BACKWARD] =
	menu_get(find_pattern, MENU_NTH_ITEM, 4); 
#ifdef _TEXTSW_FIND_RE
    menu_items[(int)TEXTSW_MENU_FIND_RE] =
	menu_get(find_pattern, MENU_NTH_ITEM, 5);
    menu_items[(int)TEXTSW_MENU_FIND_RE_BACKWARD] =
	menu_get(find_pattern, MENU_NTH_ITEM, 6);
    menu_items[(int)TEXTSW_MENU_FIND_TAG] =
	menu_get(find_pattern, MENU_NTH_ITEM, 7);
    menu_items[(int)TEXTSW_MENU_FIND_TAG_BACKWARD] =
	menu_get(find_pattern, MENU_NTH_ITEM, 8); 
#endif

    for(item = (int)TEXTSW_MENU_NO_CMD;
	item < (int)TEXTSW_MENU_LAST_CMD;
	item++) {
        menu_set(menu_items[item],
        		MENU_ACTION, textsw_do_menu_action, 0);
    }

    return menu_items;
}
static struct pixrect *
textsw_init_get_only_pr(menu)
    Menu	menu;
{
    struct pixrect		*pr;
    struct pixfont		*menu_font;
    struct pr_subregion		 get_only_bound;
    struct pr_prpos		 get_only_prpos;
    int				 menu_left_margin;
    
    /* compute the size of the pixrect and allocate it */
    menu_font = (struct pixfont *)LINT_CAST(menu_get(menu, MENU_FONT));
    pf_textbound(&get_only_bound,
	strlen(put_then_get), menu_font, put_then_get);
    menu_left_margin = (int)menu_get(menu, MENU_LEFT_MARGIN);
    pr = (struct pixrect *)mem_create(
	get_only_bound.size.x + menu_left_margin +
	(int)menu_get(menu, MENU_RIGHT_MARGIN),
	get_only_bound.size.y, 1);
    /* write the text into the pixrect */
    get_only_prpos.pr = pr;
    get_only_prpos.pos.x = menu_left_margin
	- menu_font->pf_char[put_then_get[0]].pc_home.x;
    get_only_prpos.pos.y =
	- menu_font->pf_char[put_then_get[0]].pc_home.y;
    pf_text(get_only_prpos, PIX_SRC, menu_font, put_then_get);
    /* gray out "Put, then" */
    pf_textbound(&get_only_bound,
	strlen(put_then), menu_font, put_then);
    pr_replrop(pr, menu_left_margin, 0,
	       get_only_bound.size.x, get_only_bound.size.y, 
	       PIX_SRC & PIX_DST, &menu_gray50_pr, 0, 0);
    return pr;
}

extern Menu
textsw_menu_init(textsw)
    Textsw_folio textsw;
{
    if (textsw_menu != 0) {
        textsw->menu = (caddr_t)textsw_menu;
    } else {
	(Menu)textsw->menu = textsw_menu = textsw_new_menu();
	if (textsw_get_only_pr == 0) {
	    textsw_get_only_pr = textsw_init_get_only_pr(textsw_menu);
	}
	textsw_menu_items = textsw_new_menu_table(textsw);
	textsw_menu_refcount = 0;
    }
    textsw->menu_table = (caddr_t)textsw_menu_items;
    textsw_menu_refcount++;
    return (textsw->menu);
}

extern Menu
textsw_get_unique_menu(textsw)
    Textsw_folio textsw;
{
    if (textsw->menu == textsw_menu) {
        /* refcount == 1 ==> textsw is the only referencer */
        if (textsw_menu_refcount == 1) {
            textsw_menu = 0;
            textsw_menu_items = 0;
            textsw_menu_refcount = 0;
        } else {
	    textsw->menu = (caddr_t)textsw_new_menu();
	    textsw->menu_table = (caddr_t)textsw_new_menu_table(textsw);
	    textsw_menu_refcount--;
        }
    }
    return (textsw->menu);
}

#define MENU_ACTIVE	MENU_INACTIVE, FALSE

textsw_do_menu(view, ie)
    Textsw_view		 view;
    Event		*ie;
{
    Ev_handle		 current_view;
    int			 i;
    register Textsw_folio textsw = FOLIO_FOR_VIEW(view);
    Textsw		 abstract = VIEW_REP_TO_ABS(view);
    register Menu_item	*menu_items;

    if (textsw->menu == 0) return;
    menu_items = (Menu_item *)LINT_CAST(textsw->menu_table);
    for (i = 0; i < (int)TEXTSW_MENU_LAST_CMD; i++)
	menu_set(menu_items[i],	MENU_INACTIVE,		TRUE,
				MENU_CLIENT_DATA,	abstract,
				0);

    /* Construct a menu appropriate to the the view it is invoked over.  */
    if ((textsw->state & TXTSW_NO_LOAD) == 0)
	menu_set(menu_items[(int)TEXTSW_MENU_LOAD], MENU_ACTIVE, 0);
    if (textsw->views->first_view != EV_NULL) {
	Es_handle		 original;
	original = (Es_handle)LINT_CAST(
		   es_get(textsw->views->esh, ES_PS_ORIGINAL) );
	if ((!TXTSW_IS_READ_ONLY(textsw)) &&
	    (original != ES_NULL) &&
	    (Es_enum)LINT_CAST(es_get(original, ES_TYPE)) == ES_TYPE_FILE) {
	    menu_set(menu_items[(int)TEXTSW_MENU_SAVE], MENU_ACTIVE, 0);
	    menu_set(menu_items[(int)TEXTSW_MENU_SAVE_QUIT],
		     MENU_ACTIVE, 0);
	    menu_set(menu_items[(int)TEXTSW_MENU_SAVE_RESET],
		     MENU_ACTIVE, 0);
	    menu_set(menu_items[(int)TEXTSW_MENU_SAVE_CLOSE],
		     MENU_ACTIVE, 0);
	}
	if ((textsw->state & TXTSW_READ_ONLY_SW) == 0) {
	    if (!(textsw->state & TXTSW_NO_RESET_TO_SCRATCH)
	    ||  textsw_has_been_modified(abstract))
		menu_set(menu_items[(int)TEXTSW_MENU_RESET], MENU_ACTIVE, 0);
	    menu_set(menu_items[(int)TEXTSW_MENU_STORE], MENU_ACTIVE, 0);
	    menu_set(menu_items[(int)TEXTSW_MENU_STORE_QUIT],
		     MENU_ACTIVE, 0);
	    menu_set(menu_items[(int)TEXTSW_MENU_STORE_CLOSE],
		     MENU_ACTIVE, 0);
	}
	if (textsw->tool)
	    menu_set(menu_items[(int)TEXTSW_MENU_CREATE_VIEW],
		     MENU_ACTIVE, 0);
	if ((textsw->state & TXTSW_NO_CD) == 0)
	    menu_set(menu_items[(int)TEXTSW_MENU_CD], MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_NORMALIZE_INSERTION],
		 MENU_ACTIVE, 0);
/*
	menu_set(menu_items[(int)TEXTSW_MENU_NORMALIZE_SELECTION],
		 MENU_ACTIVE, 0);
*/
	menu_set(menu_items[(int)TEXTSW_MENU_NORMALIZE_LINE],
		     MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_COUNT_TO_LINE],
		     MENU_ACTIVE, 0);
	if (!TXTSW_IS_READ_ONLY(textsw)) {
	    menu_set(menu_items[(int)TEXTSW_MENU_FILE_STUFF],
		     MENU_ACTIVE, 0);
	    /* if there is a non-zero primary selection,
	     * set string to be put_then_get, and make active;
	     * else if there is a non-zero shelf,
	     * set string to be get_only, and make active;
	     * else set string to be put_then_get, and make inactive.
	     */
	    if (textsw_is_seln_nonzero(textsw, EV_SEL_PRIMARY)) {
	        menu_set(menu_items[(int)TEXTSW_MENU_PUT_THEN_GET],
	        	 MENU_ACTIVE,
	        	 MENU_IMAGE,  0,
	        	 MENU_STRING, put_then_get,
	        	 0);
	    } else if (textsw_is_seln_nonzero(textsw, EV_SEL_SHELF)) {
	        menu_set(menu_items[(int)TEXTSW_MENU_PUT_THEN_GET],
	        	 MENU_ACTIVE,
	        	 MENU_STRING, 0,
	        	 MENU_IMAGE,  textsw_get_only_pr,
	        	 0);
	    } else {
		/* Leave MENU_INACTIVE */
	        menu_set(menu_items[(int)TEXTSW_MENU_PUT_THEN_GET],
	        	 MENU_IMAGE,  0,
	        	 MENU_STRING, put_then_get,
	        	 0);
	    }
	}
	if (textsw->views->first_view->next != EV_NULL)
	    menu_set(menu_items[(int)TEXTSW_MENU_DESTROY_VIEW],
		     MENU_ACTIVE, 0);
	current_view = view->e_view;
	if (current_view != EV_NULL) {
	    if ((Ev_right_break)ev_get(current_view, EV_RIGHT_BREAK) ==
		 EV_CLIP) {
		menu_set(menu_items[(int)TEXTSW_MENU_WRAP_LINES_AT_CHAR],
			 MENU_ACTIVE, 0);
/* 
		menu_set(menu_items[(int)TEXTSW_MENU_WRAP_LINES_AT_WORD],
			 MENU_ACTIVE, 0);
*/
	    } else
		menu_set(menu_items[(int)TEXTSW_MENU_CLIP_LINES],
			 MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_FIND], MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_FIND_BACKWARD], MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_FIND_SHELF], MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_FIND_SHELF_BACKWARD], MENU_ACTIVE,
		 0);
#ifdef _TEXTSW_FIND_RE
	menu_set(menu_items[(int)TEXTSW_MENU_FIND_RE], MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_FIND_RE_BACKWARD], MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_FIND_TAG], MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_FIND_TAG_BACKWARD], MENU_ACTIVE,
		 0);
#endif
	}
    }
    menu_show_using_fd(textsw->menu, view->window_fd, ie);
}

textsw_do_menu_action(cmd_menu, cmd_item)
    Menu		 cmd_menu;
    Menu_item		 cmd_item;
{
    extern void		  textsw_find_selection_and_normalize();
    extern void		  textsw_reset();
    Textsw		  abstract = (Textsw)LINT_CAST(
				menu_get(cmd_item, MENU_CLIENT_DATA, 0));
    register Textsw_view  view;
    register Textsw_folio textsw;
    Textsw_menu_cmd	  cmd = (Textsw_menu_cmd)
				menu_get(cmd_item, MENU_VALUE, 0);
    register Event	 *ie = (Event *)LINT_CAST(
			       menu_get(cmd_menu, MENU_FIRST_EVENT, 0) );
    register int	  locx, locy;
    Event		  event;
    register long unsigned
			  find_options;
    Es_index		  first, last_plus_one;

    if AN_ERROR(abstract == 0)
	return;

    view = VIEW_ABS_TO_REP(abstract);
    textsw = FOLIO_FOR_VIEW(view);

    if AN_ERROR(ie == 0) {
	locx = locy = 0;
    } else {
	locx = ie->ie_locx;
	locy = ie->ie_locy;
    }

    switch (cmd) {

      case TEXTSW_MENU_RESET: {
	if (textsw_has_been_modified(abstract)) {
	    textsw_menu_prompt(view, &event,
			       locx, locy, reset_prompt);
	    if (event.ie_code != MS_LEFT) {
		break;
	    }
	}
Reset:	textsw_reset(abstract, locx, locy);
	break;
      }

      case TEXTSW_MENU_LOAD: {
	int		no_cd;

	if (textsw_has_been_modified(abstract)) {
	    textsw_menu_prompt(view, &event,
			       locx, locy, load_prompt);
	    if (event.ie_code != MS_LEFT)
		break;
	}
	no_cd = ((textsw->state & TXTSW_NO_CD) ||
		 ((textsw->state & TXTSW_LOAD_CAN_CD) == 0));
	if (textsw_load_selection(textsw, locx, locy, no_cd) == 0) {
	    /* Load succeeded */
	}
	break;
      }

      case TEXTSW_MENU_SAVE_CLOSE:
	textsw_notify(view, TEXTSW_ACTION_TOOL_CLOSE, 0);
	/* Fall through */
      case TEXTSW_MENU_SAVE:
      case TEXTSW_MENU_SAVE_QUIT:
      case TEXTSW_MENU_SAVE_RESET: {
	Es_status	status;
	status = textsw_save(VIEW_REP_TO_ABS(view), locx, locy);
	if (status == ES_SUCCESS) {
	    if (cmd == TEXTSW_MENU_SAVE_QUIT) {
		textsw_notify(view, TEXTSW_ACTION_TOOL_DESTROY, (char *)ie, 0);
	    } else if (cmd == TEXTSW_MENU_SAVE_RESET) {
		goto Reset;
	    }
	}
	break;
      }

      case TEXTSW_MENU_STORE_CLOSE:
	textsw_notify(view, TEXTSW_ACTION_TOOL_CLOSE, 0);
      case TEXTSW_MENU_STORE:
      case TEXTSW_MENU_STORE_QUIT: {
	Es_status	status;
	status = textsw_store_to_selection(textsw, locx, locy);
	if ((status == ES_SUCCESS) && (cmd == TEXTSW_MENU_STORE_QUIT)) {
	    textsw_notify(view, TEXTSW_ACTION_TOOL_DESTROY, (char *)ie, 0);
	}
	break;
      }

      case TEXTSW_MENU_CREATE_VIEW: {
	extern Textsw	textsw_split_view();
	textsw_menu_prompt(view, &event,
			   locx, locy, create_view_prompt);
	if (event.ie_code == MS_LEFT) {
	    (void) textsw_split_view(VIEW_REP_TO_ABS(view),
				     event.ie_locx, event.ie_locy);
	}
	break;
      }
      
      case TEXTSW_MENU_DESTROY_VIEW: {
	extern void	textsw_destroy_split();
	(void) textsw_destroy_split(VIEW_REP_TO_ABS(view),
				    locx, locy);
	break;
      }

      case TEXTSW_MENU_CLIP_LINES:
	textsw_set(VIEW_REP_TO_ABS(view),
		   TEXTSW_LINE_BREAK_ACTION, TEXTSW_CLIP,
		   0);
	break;

      case TEXTSW_MENU_WRAP_LINES_AT_CHAR:
/*      case TEXTSW_MENU_WRAP_LINES_AT_WORD: */
	textsw_set(VIEW_REP_TO_ABS(view),
		   TEXTSW_LINE_BREAK_ACTION, TEXTSW_WRAP_AT_CHAR,
		   0);
	break;

      case TEXTSW_MENU_NORMALIZE_INSERTION: {
	extern Es_index	ev_get_insert();
	Es_index	insert;
	int		upper_context;
	insert = ev_get_insert(textsw->views);
	if (insert != ES_INFINITY) {
	    upper_context = (int)LINT_CAST(
			    ev_get(textsw->views, EV_CHAIN_UPPER_CONTEXT) );
	    textsw_normalize_internal(view, insert, insert, upper_context, 0,
					TXTSW_NI_DEFAULT);
	}
	break;
      }
#ifdef TEXTSW_MENU_NORMALIZE_SELECTION
      case TEXTSW_MENU_NORMALIZE_SELECTION: {
	ev_get_selection(
	    textsw->views, &first, &last_plus_one, EV_SEL_PRIMARY);
	if (first != ES_INFINITY) {
	    textsw_normalize_view(VIEW_REP_TO_ABS(view), first);
	}
	break;
      }
#endif
      case TEXTSW_MENU_NORMALIZE_LINE: {
	Es_index	prev;
	char		buf[10];
	unsigned	buf_fill_len;
	int		line_number;

	line_number =
	    textsw_get_selection_as_int(textsw, EV_SEL_PRIMARY, 0);
	if (line_number == 0)
		goto Out_Of_Range;
	else {
	    buf[0] = '\n'; buf_fill_len = 1;
	    if (line_number == 1) {
		prev = 0;
	    } else {
		ev_find_in_esh(textsw->views->esh, buf, buf_fill_len,
	            (Es_index)0, (u_int)line_number-1, 0, &first, &prev);
		if (first == ES_CANNOT_SET)
		    goto Out_Of_Range;
	    }
	    ev_find_in_esh(textsw->views->esh, buf, buf_fill_len,
			    prev, 1, 0, &first, &last_plus_one);
	    if (first == ES_CANNOT_SET)
		goto Out_Of_Range;
	    textsw_possibly_normalize_and_set_selection(
		VIEW_REP_TO_ABS(view), prev, last_plus_one,
		EV_SEL_PRIMARY);
	    (void) textsw_set_insert(textsw, last_plus_one);
	}
Out_Of_Range:
	break;
      }
      case TEXTSW_MENU_COUNT_TO_LINE: {
	int		count;
	char		msg[200];

	ev_get_selection(
	    textsw->views, &first, &last_plus_one, EV_SEL_PRIMARY);
	if (first >= last_plus_one)
	    break;
	count = ev_newlines_in_esh(textsw->views->esh, 0, first);
	(void) sprintf(msg, "Selection starts in line %d.  %s", count+1,
		    "Click any button to remove the message.");
	textsw_menu_prompt(view, &event,
			    locx, locy, msg);
	break;
      }
      case TEXTSW_MENU_FILE_STUFF:
	textsw_file_stuff(view, locx, locy);
	break;

      case TEXTSW_MENU_CD:
	textsw_cd(textsw, locx, locy);
	break;

      case TEXTSW_MENU_FIND_BACKWARD:
	find_options = TFSAN_BACKWARD;
	/* Fall through */
      case TEXTSW_MENU_FIND:
	find_options |= (EV_SEL_PRIMARY | TFSAN_SHELF_ALSO);
	textsw_find_selection_and_normalize(view, locx, locy, find_options);
	break;

      case TEXTSW_MENU_FIND_SHELF_BACKWARD:
	find_options = TFSAN_BACKWARD;
	/* Fall through */
      case TEXTSW_MENU_FIND_SHELF:
	find_options |= EV_SEL_SHELF;
	textsw_find_selection_and_normalize(view, locx, locy, find_options);
	break;

#ifdef _TEXTSW_FIND_RE
      case TEXTSW_MENU_FIND_RE_BACKWARD:
	find_options = TFSAN_BACKWARD;
	/* Fall through */
      case TEXTSW_MENU_FIND_RE:
	find_options |= (EV_SEL_PRIMARY | TFSAN_REG_EXP | TFSAN_SHELF_ALSO);
	textsw_find_selection_and_normalize(view, locx, locy, find_options);
	break;

      case TEXTSW_MENU_FIND_TAG_BACKWARD:
	find_options = TFSAN_BACKWARD;
	/* Fall through */
      case TEXTSW_MENU_FIND_TAG:
	find_options |=
	    (EV_SEL_PRIMARY | TFSAN_REG_EXP | TFSAN_SHELF_ALSO | TFSAN_TAG);
	textsw_find_selection_and_normalize(view, locx, locy, find_options);
	break;
#endif

      case TEXTSW_MENU_PUT_THEN_GET:
        textsw_put_then_get(view);
	break;

      default:
	break;
    }
}

static int
prompt_user(windowfd, result_event, locx, locy, msg)
	int			 windowfd;
	Event			*result_event;
	int			 locx, locy;
	char			*msg;
{
	extern struct pixfont	*pw_pfsysopen();
	struct prompt		 prompt;
	int			 result;

	rect_construct(&prompt.prt_rect, locx+16, locy,
			PROMPT_FLEXIBLE, PROMPT_FLEXIBLE);
	    /* The +16 is to move away from the cursor image. */
	prompt.prt_font = pw_pfsysopen();
	prompt.prt_text = msg;
	result = menu_prompt(&prompt, result_event, windowfd);
	pw_pfsysclose();
	return(result);
}

pkg_private int
await_neg_event(windowfd, event)
	int			 windowfd;
	struct inputevent	*event;
{
	struct fullscreen	*fsh;
	struct inputevent	 new_event;

	fsh = fullscreen_init(windowfd);
	if (fsh == 0)
	    return(1);
	FOREVER {
	    if AN_ERROR(input_readevent(windowfd, &new_event) == -1) {
	        break;
	    }
	    if (new_event.ie_code == event->ie_code
	    &&  win_inputnegevent(&new_event))
	        break;
	}
	fullscreen_destroy(fsh);
	return(0);
}

extern int
fd_menu_prompt(fd, event, locx, locy, msg)
	int			 fd;
	Event			*event;
	int			 locx, locy;
	char			*msg;
{
	extern int		 gettimeofday();
	int			 designee;
	struct inputmask	 mask;


	win_getinputmask(fd, &mask, &designee);
	textsw_set_mouse_button_mask(fd);
	/* drain fd */
	while(input_readevent(fd, event) != -1) {
	}
	if (prompt_user(fd, event, locx, locy, msg))
	    /* Probably failed to display msg due to lack of fds. */
	    return(1);
	/* Following approximates "wait until all keystations go up". */
	if (win_inputposevent(event) && !event_is_ascii(event)) {
	    await_neg_event(fd, event);
	}
	win_setinputmask(fd, &mask, (struct inputmask *)0, designee);
	return(0);
}

pkg_private int
textsw_menu_prompt(view, event, locx, locy, msg)
	Textsw_view		 view;
	struct inputevent	*event;
	int			 locx, locy;
	char			*msg;
{
	int			 result;

	textsw_may_win_exit(FOLIO_FOR_VIEW(view));
	    /*
	     * The above is needed because we may miss a WIN_EXIT as a result
	     * of calling prompt_user.
	     */
	result = fd_menu_prompt(view->window_fd, event, locx, locy, msg);
	if (result) {
	    (void) fprintf(stderr, "[textsw_menu_prompt] %s\n", msg);
	    event->ie_code = MS_RIGHT;		/* Fake user abort. */
	}
	return(result);
}

/* ARGSUSED */
extern Textsw
textsw_split_view(abstract, locx, locy)
	Textsw			abstract;
	int			locx, locy;	/* locx currently unused */
{
	extern struct toolsw	*tool_find_sw_with_client();
	extern Textsw_view	 textsw_create_view();
	extern caddr_t		 textsw_view_window_object();
	register struct toolsw	*toolsw;
	struct rect		 new_to_split_rect,
				 new_view_rect;
	int			 left_margin, right_margin, start_line;
	register int		 to_split_ts_height, to_split_ts_width;
	Es_index		 start_pos;
	register Textsw_view	 new_view;
	register Textsw_view	 to_split = VIEW_ABS_TO_REP(abstract);
	Textsw_folio		 folio = FOLIO_FOR_VIEW(to_split);

	/*
	 * Only split views that are large enough
	 */
	if ((locy < 0) || (to_split->rect.r_height < locy) ||
	    (textsw_screen_line_count(abstract) < 3)) {
	    return(TEXTSW_NULL);
	}
	win_getrect(to_split->window_fd, &new_to_split_rect);
	new_view_rect.r_left = new_to_split_rect.r_left;
	new_view_rect.r_width = new_to_split_rect.r_width;
	new_view_rect.r_height = to_split->rect.r_height - locy;
	if (new_view_rect.r_height < ei_line_height(folio->views->eih))
	    new_view_rect.r_height = ei_line_height(folio->views->eih);
	new_view_rect.r_top =
	    rect_bottom(&new_to_split_rect) + 1 - new_view_rect.r_height;
	/*
	 * start_line/pos must be determined BEFORE adjusting to_split.
	 */
	start_line = ev_line_for_y(to_split->e_view,
				   new_view_rect.r_top -
					new_to_split_rect.r_top);
	start_pos = ev_index_for_line(to_split->e_view, start_line);
	if (start_pos == ES_INFINITY) {
	    start_pos = ev_index_for_line(to_split->e_view, 0);
	}
	new_to_split_rect.r_height -=
	    new_view_rect.r_height + TOOL_SUBWINDOWSPACING;
	win_setrect(to_split->window_fd, &new_to_split_rect);
	toolsw = tool_find_sw_with_client((Tool *)LINT_CAST(folio->tool),
	    (caddr_t)LINT_CAST(to_split));
	if (toolsw) {
	    to_split_ts_height = toolsw->ts_height;
	    to_split_ts_width = toolsw->ts_width;
	    toolsw->ts_height = new_to_split_rect.r_height;
	}
	left_margin = (int)ev_get(to_split->e_view, EV_LEFT_MARGIN);
	right_margin = (int)ev_get(to_split->e_view, EV_RIGHT_MARGIN);
	new_view = textsw_create_view(folio, new_view_rect,
					left_margin, right_margin);
	if (to_split_ts_height == TOOL_SWEXTENDTOEDGE ||
	    to_split_ts_width == TOOL_SWEXTENDTOEDGE) {
	    toolsw = tool_find_sw_with_client((Tool *)LINT_CAST(folio->tool),
		(caddr_t)LINT_CAST(new_view));
	    if (toolsw) {
		if (to_split_ts_height == TOOL_SWEXTENDTOEDGE)
		    toolsw->ts_height = TOOL_SWEXTENDTOEDGE;
		if (to_split_ts_width == TOOL_SWEXTENDTOEDGE)
		    toolsw->ts_width = TOOL_SWEXTENDTOEDGE;
	    }
	}
	if (SCROLLBAR_FOR_VIEW(to_split)) {
	    extern void textsw_add_scrollbar_to_view();

	    textsw_add_scrollbar_to_view(new_view, TEXTSW_DEFAULT_SCROLLBAR);
	}
	(void) ev_set(new_view->e_view, EV_SAME_AS, to_split->e_view, 0);
	
	/*  
	 *  Although this call to ev_set_start will cause an unnecessary
	 *  display of the newly created view, but it is necessary
	 *  to update the line table of the new view.
	 */
	ev_set_start(new_view->e_view, start_pos); 

	window_create((Window)LINT_CAST(window_get(WINDOW_FROM_VIEW(to_split),
			  WIN_OWNER)),
		      textsw_view_window_object, WIN_COMPATIBILITY,
		      WIN_OBJECT, new_view,
		      0);
	/* BUG ALERT - why isn't the system doing this for us? */
	if (win_fdtonumber(to_split->window_fd)
	== win_get_kbd_focus(to_split->window_fd)
	&& folio->tool != 0)
	    tool_displaytoolonly(TOOL_FROM_FOLIO(folio));
	if (folio->notify_level & TEXTSW_NOTIFY_SPLIT_VIEW)
	    textsw_notify(to_split, TEXTSW_ACTION_SPLIT_VIEW, new_view, 0);
	ASSERT(allock());
	return(VIEW_REP_TO_ABS(new_view));
}

static Textsw_view
textsw_adjacent_view(view, coalesced_rect)
	register Textsw_view	 view;
	register Rect		*coalesced_rect;
{
	Textsw_folio		 folio = FOLIO_FOR_VIEW(view);
	Textsw_view		 above = (Textsw_view)0;
	Textsw_view		 under = (Textsw_view)0;
	register Textsw_view	 other;
	Rect			 view_rect, other_rect;

	win_getrect(WIN_FD_FOR_VIEW(view), &view_rect);
	FORALL_TEXT_VIEWS(folio, other) {
	    if (view == other)
		continue;
	    win_getrect(WIN_FD_FOR_VIEW(other), &other_rect);
	    if ((view_rect.r_left == other_rect.r_left) &&
		(view_rect.r_width == other_rect.r_width)) {
		/* Possible vertical split */
		if ((rect_bottom(&view_rect) + 1 +
		    TOOL_SUBWINDOWSPACING)
		==  other_rect.r_top) {
		    under = other;
		    *coalesced_rect = view_rect;
		    coalesced_rect->r_height =
			rect_bottom(&other_rect) + 1 - coalesced_rect->r_top;
		} else if ((rect_bottom(&other_rect) + 1 +
		    TOOL_SUBWINDOWSPACING)
		==  view_rect.r_top) {
		    above = other;
		    *coalesced_rect = other_rect;
		    coalesced_rect->r_height =
			rect_bottom(&view_rect) + 1 - coalesced_rect->r_top;
		    break;
		}
	    }
	}
	return((above) ? above : under);
}

static Textsw_view
textsw_wrapped_adjacent_view(view, rect)
    register Textsw_view	 view;
    register Rect		*rect;
{
    Textsw_view	 upper_view;
    Textsw_view	 coalesce_with = 0;

    coalesce_with = textsw_adjacent_view(view, rect);
    if (coalesce_with) {
	if ((int)window_get(WINDOW_FROM_VIEW(view), WIN_Y)
	<   (int)window_get(WINDOW_FROM_VIEW(coalesce_with), WIN_Y)) {
	    upper_view = view;
	} else {
	    upper_view = coalesce_with;
	}
	rect->r_top = (int)LINT_CAST(window_get(WINDOW_FROM_VIEW(upper_view),
	    WIN_Y));
	rect->r_left = (int)LINT_CAST(window_get(WINDOW_FROM_VIEW(upper_view),
	    WIN_X));
    } else {
	win_getrect((int)window_get(WINDOW_FROM_VIEW(view), WIN_FD), rect);
    }
    return (coalesce_with);
}

static Notify_value
textsw_menu_destroy_view(abstract, status)
    Textsw		abstract;
    Destroy_status	status;
{
    caddr_t	 frame = window_get(abstract, WIN_OWNER);
    Textsw_view	 view = VIEW_ABS_TO_REP(abstract);
    Textsw_folio folio = FOLIO_FOR_VIEW(view);
    Rect	 rect;
    int		 win_show;
    int		 had_kbd_focus;
    Notify_value nv;
    int		 height, width;

    if (status != DESTROY_CLEANUP) {
	return (notify_next_destroy_func((Notify_client)abstract, status));
    }
    /* Look to see if we can coalesce with one of our own views. */
    folio->coalesce_with = textsw_wrapped_adjacent_view(view, &rect);
    textsw_get_extend_to_edge(view, &height, &width);
    
    win_show = (int)LINT_CAST(window_get(WINDOW_FROM_VIEW(view), WIN_SHOW));
    had_kbd_focus = (int)LINT_CAST(window_get(WINDOW_FROM_VIEW(view),
	    WIN_KBD_FOCUS));

    /* Unlink view from everywhere and remove its storage */
    nv = notify_next_destroy_func((Notify_client)abstract, status);
    
    if (folio->coalesce_with) {
	if ((int)LINT_CAST(window_get(WINDOW_FROM_VIEW(folio->coalesce_with),
	    WIN_SHOW)) && !win_show) {
	    window_set(WINDOW_FROM_VIEW(folio->coalesce_with),
	    	       WIN_SHOW, win_show, 0);
	} else {
	    if (had_kbd_focus) {
		window_set(folio->coalesce_with, WIN_KBD_FOCUS, TRUE, 0);
	    }
	}
	window_set(WINDOW_FROM_VIEW(folio->coalesce_with), 
		   WIN_RECT, &rect, 0);
	textsw_set_extend_to_edge(folio->coalesce_with, height, width);
		   
	if (win_fdtonumber(folio->coalesce_with->window_fd)
	==  win_get_kbd_focus(folio->coalesce_with->window_fd)) {
	    tool_displaytoolonly(TOOL_FROM_FOLIO(folio));
	}
    } else if (folio->tool) {
	tool_expand_neighbors(TOOL_FROM_FOLIO(folio), TOOLSW_NULL, &rect);
    }
    folio->coalesce_with = 0;
    return (nv);
}

static Notify_value
textsw_menu_destroy_first(abstract, status)
    Textsw		abstract;
    Destroy_status	status;
{
    caddr_t	 frame = window_get(abstract, WIN_OWNER);
    Textsw_view	 tagged_view = VIEW_ABS_TO_REP(abstract);
    Textsw_view	 first_view;
    Textsw_folio folio = FOLIO_FOR_VIEW(tagged_view);
    Rect	 rect;
    Rect	 empty_rect;
    Es_index	 tagged_view_first = ES_CANNOT_SET;
    int		 win_show;
    int		 had_kbd_focus;
    Notify_value nv;
    int		 height, width;

    if (status != DESTROY_CLEANUP) {
	return (notify_next_destroy_func((Notify_client)abstract, status));
    }
    first_view = folio->first_view;
    /* Look to see if we can coalesce with one of our own views. */
    folio->coalesce_with = textsw_wrapped_adjacent_view(first_view, &rect);
    if (folio->coalesce_with) {
	/* ASSERT(folio->coalesce_with == tagged_view); */
	/* 
	 * At this point folio->coalesce_with should point to the first view.
	 * tagged_view is pointing to the view going to be destroyed.
	 */
	textsw_get_extend_to_edge(folio->coalesce_with, &height, &width);
	folio->coalesce_with = first_view;
    } else {
	empty_rect = rect;
	rect = *((Rect *)LINT_CAST(window_get(WINDOW_FROM_VIEW(tagged_view),
	    WIN_RECT)));
	 textsw_get_extend_to_edge(tagged_view, &height, &width);   
    }
    win_show = (int)LINT_CAST(window_get(WINDOW_FROM_VIEW(tagged_view),
	    WIN_SHOW));
    had_kbd_focus = (int)LINT_CAST(window_get(WINDOW_FROM_VIEW(tagged_view),
	    WIN_KBD_FOCUS));
    tagged_view_first
	= (Es_index)textsw_get(abstract, TEXTSW_FIRST);
    if (tagged_view_first == (Es_index)LINT_CAST(textsw_get(
	VIEW_REP_TO_ABS(first_view), TEXTSW_FIRST))) {
	tagged_view_first = ES_CANNOT_SET;
    }

    /* Unlink view from everywhere and remove its storage */
    nv = notify_next_destroy_func((Notify_client)abstract, status);

    if ((int)LINT_CAST(window_get(WINDOW_FROM_VIEW(first_view), WIN_SHOW)) &&
	!win_show) {
	window_set(WINDOW_FROM_VIEW(first_view), WIN_SHOW, win_show, 0);
    } else {
	if (had_kbd_focus) {
	    window_set(first_view, WIN_KBD_FOCUS, TRUE, 0);
	}
    }
    if (tagged_view_first != ES_CANNOT_SET) {
	textsw_set(VIEW_REP_TO_ABS(first_view),
		   TEXTSW_FIRST, tagged_view_first, 0);
    }
    window_set(WINDOW_FROM_VIEW(first_view), WIN_RECT, &rect, 0);
    textsw_set_extend_to_edge(first_view, height, width);
    
    if (win_fdtonumber(first_view->window_fd)
    ==  win_get_kbd_focus(first_view->window_fd)) {
	tool_displaytoolonly(TOOL_FROM_FOLIO(folio));
    }
    if (folio->coalesce_with) {
	folio->coalesce_with = 0;
    } else {
	tool_expand_neighbors(TOOL_FROM_FOLIO(folio), TOOLSW_NULL,&empty_rect);
    }
    return (nv);
}

/* ARGSUSED */
extern void
textsw_destroy_split(abstract, locx, locy)
	Textsw		 abstract;
	int		 locx, locy;	/* Currently unused */
{
	Textsw_view	 designated_view = VIEW_ABS_TO_REP(abstract);
	Textsw_view	 tagged_view;
	Textsw_view	 adjacent_view;
	Textsw_folio	 folio = FOLIO_FOR_VIEW(designated_view);
	Rect		 rect;

	if (folio->first_view == designated_view) {
	    if (!designated_view->next)
		return;
	    adjacent_view = textsw_adjacent_view(designated_view, &rect);
	    if (adjacent_view) {
		tagged_view = adjacent_view;
	    } else {
		tagged_view = designated_view->next;
	    }
	    notify_interpose_destroy_func(
	        (Notify_client)tagged_view, textsw_menu_destroy_first);
	} else {
	    tagged_view = designated_view;
	    notify_interpose_destroy_func(
	        (Notify_client)tagged_view, textsw_menu_destroy_view);
	}
	window_destroy(WINDOW_FROM_VIEW(tagged_view));
}

pkg_private void
textsw_set_extend_to_edge(view, height, width)
	Textsw_view	view;
	int		height, width;
	
{
	Textsw_folio	folio;
	register struct toolsw	*toolsw;
	
	if (view) {
	    folio = FOLIO_FOR_VIEW(view);

	    toolsw = tool_find_sw_with_client((Tool *)LINT_CAST(folio->tool),
		(caddr_t)LINT_CAST(view));
		
	    if (toolsw) {
	        if (height == TOOL_SWEXTENDTOEDGE)
	            toolsw->ts_height = TOOL_SWEXTENDTOEDGE;
	        if (width == TOOL_SWEXTENDTOEDGE)
	            toolsw->ts_width = TOOL_SWEXTENDTOEDGE;
	    }
	}	
}

pkg_private void
textsw_get_extend_to_edge(view, height, width)
	Textsw_view	view;
	int		*height, *width;
	
{
	Textsw_folio	folio;
	register struct toolsw	*toolsw;
	
	*height = 0;
	*width = 0;
	
	if (view) {
	    folio = FOLIO_FOR_VIEW(view);
	    toolsw = tool_find_sw_with_client((Tool *)LINT_CAST(folio->tool),
		(caddr_t)LINT_CAST(view));
	    if (toolsw) {
	        *height = toolsw->ts_height;
	        *width = toolsw->ts_width;
	    }
	}	
}

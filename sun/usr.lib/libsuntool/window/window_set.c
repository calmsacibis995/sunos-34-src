#ifndef lint
static	char sccsid[] = "@(#)window_set.c 1.4 87/01/07 Copyright 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*-
	WINDOW wrapper

	window_set.c, Sun Aug 8 15:38:39 1985

 */

/* ------------------------------------------------------------------------- */

#include <stdio.h>
#include <varargs.h>

#include <sys/file.h>
#include <sys/types.h>
#include <sys/time.h>

#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>

#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_struct.h>

#include "window_impl.h"

/* ------------------------------------------------------------------------- */

/* 
 * Public
 */


/* 
 * Package private
 */

Pkg_private int/*bool*/ 	window_set();
Pkg_private int/*bool*/		window_set_avlist();
Pkg_private int			win_appeal_to_owner();
Pkg_private void		window_rc_units_to_pixels();


Pkg_extern Notify_value 	window_default_event_func();
Pkg_extern Notify_value 	window_default_destroy_func();
Pkg_extern Window		win_set_client();


/* 
 * Private
 */

Private void			window_scan_and_convert_to_pixels();
Private void 			set_mask_bit(), unset_mask_bit();

/* ------------------------------------------------------------------------- */

void	wmgr_changelevelonly();

/*VARARGS1*/
Pkg_private int/*bool*/
window_set(window, va_alist)
	Window window;
	va_dcl
{
    register struct window *win;
    va_list valist;
    int count;
   /*
    * FIXME: The first two slot of the avlist are used for next_pos, free_slots.
    *        This would be cleaner if a separate struct was used.
    */

    win = client_to_win(window);
    if (!win) return FALSE;

    if (win->set_private) {
	int *avlist = (int *)(LINT_CAST(win->set_private));
	
	va_start(valist);
	(void)attr_make_count((char **)(&avlist[avlist[0]]), avlist[1],
			valist, &count);
	va_end(valist);
	--count;	/* Do not include trailing null */

	window_rc_units_to_pixels(win->object, 
		(Window_attribute *)(&avlist[avlist[0]]));
	avlist[0] += count;
	avlist[1] -= count;
	return TRUE;
    
    } else {
	int avlist[ATTR_STANDARD_SIZE << 1];
	int status = 0;
	
	avlist[0] = 2;
	avlist[1] = (ATTR_STANDARD_SIZE << 1) - avlist[0];
	avlist[2] = NULL;
	va_start(valist);
	(void)attr_make_count((char **)(&avlist[avlist[0]]), avlist[1],
			valist, &count);
	va_end(valist);
	--count;	/* Do not include trailing null */
	avlist[0] += count;
	avlist[1] -= count;

	if (win->preset_proc)
	    status &= (win->preset_proc)(win->object, &avlist[2]);

	window_scan_and_convert_to_pixels(win, (Window_attribute *)&avlist[2]);

	win->set_private = (caddr_t)avlist;
	if (win->set_proc) status &= (win->set_proc)(win->object, &avlist[2]);

	status &= window_set_avlist(win, (Window_attribute *)&avlist[2]);

	if (win->postset_proc) {
	    int end_of_list = avlist[0]; /* remember, in case of additions */
	    status &= (win->postset_proc)(win->object, &avlist[2]);
	    if (end_of_list != avlist[0])
		status &= window_set_avlist(win, 
			(Window_attribute *)(&avlist[end_of_list]));
	}	    

	win->set_private = NULL;

	return status;
    }
}


Pkg_private int/*bool*/
window_set_avlist(win, avlist)
	register struct window *win;
	Window_attribute avlist[];
{   
    register Window_attribute *attrs; 
    int	x, y;
    struct window *owner;
    caddr_t old_object, notify_flags = 0;
    int allow_registration = TRUE, do_registration = FALSE, adjusted = FALSE;
    Notify_value (*event_proc)() = window_default_event_func;
    Notify_value (*destroy_proc)() = window_default_destroy_func;
    
    if (win->fd >= 0) (void)win_lockdata(win->fd);
    for (attrs = avlist; *attrs; attrs = window_attr_next(attrs)) {
	switch (attrs[0]) {

          case WIN_PIXWIN:
            /*
            this is a hack so that the ttysw create routine can
            set the pixwin to its global csr_pixwin.  This is
            not to be called in general.
            */
            if (!(win->registered || win->created))
               win->pixwin = (struct pixwin *)attrs[1];
            break;

	  case WIN_BELOW:
	    adjusted |= win_appeal_to_owner(TRUE, win, (caddr_t)WIN_ADJUST_BELOW,
					    (caddr_t)(LINT_CAST(attrs[1])));
	    break;

	  case WIN_CLIENT_DATA:
	    win->client_data = (caddr_t)attrs[1];
	    break;

	  case WIN_COLUMNS:
	    x = (int)attrs[1] * (actual_column_width(win) + win->column_gap)
		+ win->left_margin + win->right_margin;
	    adjusted |= win_appeal_to_owner(TRUE, win,(caddr_t)WIN_ADJUST_WIDTH, 
	    		(caddr_t)(LINT_CAST(x)));
	    break;

	  case WIN_CURSOR:
	    (void)win_setcursor(win->fd, (struct cursor *)attrs[1]);
	    break;

	  case WIN_DEFAULT_EVENT_PROC:
	    win->default_event_proc = (void (*)())attrs[1];
	    if (!win->default_event_proc)
		win->default_event_proc = (void (*)())window_default_event_func;
	    break;

	  case WIN_EVENT_PROC:
	    win->event_proc = (void (*)())attrs[1];
	    break;

	  case WIN_FIT_HEIGHT:
	    if (!win->get_proc) break;
	    y = (int)(win->get_proc)(win->object, WIN_FIT_HEIGHT);
	    if (y <= 0) y = (int)window_get(win->object, WIN_HEIGHT);
	    y += (int)attrs[1]; /* Avoid lvalue cast */
	    attrs[1] = (Window_attribute)y;
	    adjusted |= win_appeal_to_owner(TRUE,win,(caddr_t)WIN_ADJUST_HEIGHT, 
	    		(caddr_t)(attrs[1]));
	    break;
	  
	  case WIN_FIT_WIDTH:
	    if (!win->get_proc) break;
	    x = (int)(win->get_proc)(win->object, WIN_FIT_WIDTH);
	    if (x <= 0) x = (int)window_get(win->object, WIN_WIDTH);
	    x += (int)attrs[1]; /* Avoid lvalue cast */
	    attrs[1] = (Window_attribute)x;
	    adjusted |= win_appeal_to_owner(TRUE, win,(caddr_t)WIN_ADJUST_WIDTH, 
	    		(caddr_t)(attrs[1]));
	    break;
	  
          /* WIN_FONT prescanned */

	  case WIN_GET_PROC:
	    win->get_proc = (caddr_t (*)())attrs[1];
	    break;

	  case WIN_HEIGHT:
	    adjusted |= win_appeal_to_owner(TRUE, win, (caddr_t)WIN_ADJUST_HEIGHT, 
	    		(caddr_t)(attrs[1]));
	    break;

	  case WIN_HORIZONTAL_SCROLLBAR:
	    break;
		
	  case WIN_IMPL_DATA:
	    win->impl_data = (caddr_t)attrs[1];
	    break;

	  case WIN_KBD_FOCUS:
	    if (attrs[1])
		(void)win_set_kbd_focus(win->fd, win_fdtonumber(win->fd));
	    else
		(void)win_set_kbd_focus(win->fd, WIN_NULLLINK);
	    break;

	  case WIN_LAYOUT_PROC:
	    win->layout_proc = (int (*)())attrs[1];
	    break;
	
	  case WIN_MENU:
	    win->menu = (caddr_t)attrs[1];
	    break;

	  case WIN_MOUSE_XY:
	    (void)win_setmouseposition(win->fd, (short)attrs[1], (short)attrs[2]);
	    break;

	  case WIN_NAME:
	    win->name = (char *)attrs[1];
	    break;

	  case WIN_NOTIFY_DESTROY_PROC:
	    destroy_proc = (Notify_value (*)())attrs[1];
	    break;

	  case WIN_NOTIFY_EVENT_PROC:
	    event_proc = (Notify_value (*)())attrs[1];
	    break;

	  case WIN_NOTIFY_INFO:
	    notify_flags = (caddr_t)attrs[1];
	    break;

	  case WIN_OBJECT:
	    old_object = win_set_client(win, (caddr_t)(LINT_CAST(attrs[1])), FALSE);
	    if (old_object) do_registration = TRUE;
	    break;

	  case WIN_PERCENT_HEIGHT:
	    owner = win->owner ? win->owner : win;
	    attrs[1] = (Window_attribute)
		((((int)window_get(owner->object, WIN_HEIGHT) -
		   owner->top_margin - owner->bottom_margin) *
		  (int)attrs[1] / 100) -
		 owner->row_gap);
	    adjusted |= win_appeal_to_owner(TRUE, win, (caddr_t)WIN_ADJUST_HEIGHT, 
	    		(caddr_t)(attrs[1]));
	    break;
	  
	  case WIN_PERCENT_WIDTH:
	    owner = win->owner ? win->owner : win;
	    attrs[1] = (Window_attribute)
		((((int)window_get(owner->object, WIN_WIDTH) -
		   owner->left_margin - owner->right_margin) *
		  (int)attrs[1] / 100) -
		 owner->column_gap);
	    adjusted |= win_appeal_to_owner(TRUE, win, (caddr_t)WIN_ADJUST_WIDTH, 
	    		(caddr_t)(attrs[1]));
	    break;
	  
	  case WIN_PRESET_PROC:
	    win->preset_proc = (int (*)())attrs[1];
	    break;

	  case WIN_POSTSET_PROC:
	    win->postset_proc = (int (*)())attrs[1];
	    break;

	  case WIN_RECT:
	    adjusted |= win_appeal_to_owner(TRUE, win, (caddr_t)WIN_ADJUST_RECT, 
	    		(caddr_t)(attrs[1]));
	    break;

	  case WIN_REGISTER:
	    allow_registration = (int)attrs[1];
	    break;

	  case WIN_RIGHT_OF:
	    adjusted |= win_appeal_to_owner(TRUE, win,(caddr_t)WIN_ADJUST_RIGHT_OF, 
	    		(caddr_t)(attrs[1]));
	    break;

	  case WIN_ROWS:
	    y = (int)attrs[1] * (actual_row_height(win) + win->row_gap)
		+ win->top_margin + win->bottom_margin;
	    adjusted |= win_appeal_to_owner(TRUE, win, (caddr_t)WIN_ADJUST_HEIGHT, 
	    		(caddr_t)(LINT_CAST(y)));
	    break;

	  case WIN_SCREEN_RECT:
	    break; /* Get only */

	  case WIN_SET_PROC:
	    win->set_proc = (int (*)())attrs[1];
	    break;

	  case WIN_SHOW:
	    if (win->show != (unsigned)attrs[1]) {
		win->show = (unsigned)attrs[1];

               /* Acts as initialize value prior to object''s existence */
		if  (win->object == (caddr_t)win) break;
		
		if (win->show) {
		    adjusted |= win_appeal_to_owner(FALSE, win, (caddr_t)WIN_INSERT, 
		    	(caddr_t)(attrs[1]));
		} else {
		    adjusted |= win_appeal_to_owner(FALSE, win, (caddr_t)WIN_REMOVE, 
		    	(caddr_t)(attrs[1]));
		}
	    }
	    
	    if (win->show) {
 	       /* Bring window to top of the heap */
		int link = win_getlink(win->fd, WL_PARENT);
		char parent_name[WIN_NAMESIZE];
		int parent_fd;
		
		(void)win_numbertoname(link, parent_name);
		parent_fd = open(parent_name, O_RDONLY, 0);
		wmgr_changelevelonly(win->fd, parent_fd, TRUE);
		(void)close(parent_fd);
	    }		
	    break;

	  /* WIN_SHOW_UPDATES prescanned */

	  case WIN_TYPE:
	    win->type = (Attr_pkg)attrs[1];
	    break;

	  case WIN_VERTICAL_SCROLLBAR:
	    break;
		
	  case WIN_WIDTH:
	    adjusted |= win_appeal_to_owner(TRUE, win, (caddr_t)WIN_ADJUST_WIDTH, 
	    		(caddr_t)(attrs[1]));
	    break;

	  case WIN_X:
	    adjusted |= win_appeal_to_owner(TRUE, win, (caddr_t)WIN_ADJUST_X, 
	    		(caddr_t)(attrs[1]));
	    break;

	  case WIN_Y:
	    adjusted |= win_appeal_to_owner(TRUE, win, (caddr_t)WIN_ADJUST_Y, 
	    		(caddr_t)(attrs[1]));
	    break;

          case WIN_INPUT_DESIGNEE:
	    (void)win_set_designee(win->fd, (int)attrs[1]);
            break;
             
	  case WIN_GRAB_ALL_INPUT:
            if (attrs[1])
               (void)win_grabio(win->fd);
            else
               (void)win_releaseio(win->fd);
            break;

          case WIN_KBD_INPUT_MASK:
            (void)win_set_kbd_mask(win->fd, (struct inputmask *)attrs[1]);
            break;
 
	  case WIN_CONSUME_KBD_EVENT:
            {
               struct inputmask mask;
               (void)win_get_kbd_mask(win->fd, &mask);
	       set_mask_bit(&mask, attrs[1]);
               (void)win_set_kbd_mask(win->fd, &mask);
            }   
	    break;

	  case WIN_IGNORE_KBD_EVENT:
            {
               struct inputmask mask;
               (void)win_get_kbd_mask(win->fd, &mask);
	       unset_mask_bit(&mask, attrs[1]);
               (void)win_set_kbd_mask(win->fd, &mask);
            }   
	    break;

          case WIN_CONSUME_KBD_EVENTS:
            {
               struct inputmask mask;
               register int i;
               (void)win_get_kbd_mask(win->fd, &mask);
               for(i=1; attrs[i]; i++)
		  set_mask_bit(&mask, attrs[i]);
               (void)win_set_kbd_mask(win->fd, &mask);
            }   
            break;
             
          case WIN_IGNORE_KBD_EVENTS:
            {
               struct inputmask mask;
               register int i;
               (void)win_get_kbd_mask(win->fd, &mask);
               for(i=1; attrs[i]; i++)
		  unset_mask_bit(&mask, attrs[i]);
               (void)win_set_kbd_mask(win->fd, &mask);
            }   
            break;
             
          case WIN_PICK_INPUT_MASK:
            (void)win_set_pick_mask(win->fd, (struct inputmask *)attrs[1]);
            break;
 
	  case WIN_CONSUME_PICK_EVENT:
            {
               struct inputmask mask;
               (void)win_get_pick_mask(win->fd, &mask);
	       set_mask_bit(&mask, attrs[1]);
               (void)win_set_pick_mask(win->fd, &mask);
            }   
	    break;

	  case WIN_IGNORE_PICK_EVENT:
            {
               struct inputmask mask;
               (void)win_get_pick_mask(win->fd, &mask);
	       unset_mask_bit(&mask, attrs[1]);
               (void)win_set_pick_mask(win->fd, &mask);
            }   
	    break;

          case WIN_CONSUME_PICK_EVENTS:
            {
               struct inputmask mask;
               register int i;
               (void)win_get_pick_mask(win->fd, &mask);
               for(i=1; attrs[i]; i++)
		  set_mask_bit(&mask, attrs[i]);
               (void)win_set_pick_mask(win->fd, &mask);
            }   
            break;
             
          case WIN_IGNORE_PICK_EVENTS:
            {
               struct inputmask mask;
               register int i;
               (void)win_get_pick_mask(win->fd, &mask);
               for(i=1; attrs[i]; i++)
		  unset_mask_bit(&mask, attrs[i]);
               (void)win_set_pick_mask(win->fd, &mask);
            }   
            break;
             
	  case WIN_ERROR_MSG:	 /* Ignore this attr during set */
	    break;
	  
	  /*
	   *   There is abug dealing with nested calls to window_set()
	   *   from an object set_proc.
	   *   Note:  This is a fix for frame_set() that allows frame_set()
	   *   to set the TOP_MARGIN
	   */
	  case WIN_TOP_MARGIN:
	    win->top_margin = (int)attrs[1];
	    break;
	  
	  case WIN_FONT:	 /* These attrs have been prescanned */
	  case WIN_BOTTOM_MARGIN:
	  case WIN_LEFT_MARGIN:
	  case WIN_RIGHT_MARGIN:
	  case WIN_ROW_HEIGHT:
	  case WIN_COLUMN_WIDTH:
	  case WIN_ROW_GAP:
	  case WIN_COLUMN_GAP:
	  case WIN_SHOW_UPDATES:
	  case WIN_COMPATIBILITY:
	  case WIN_COMPATIBILITY_INFO:
	    break;

	  case ATTR_LIST:
	    /* This only occurs when a pkg_set wants to do inline */
	    /* substitution.  Don''t defer the set. */
	    if (attrs[1]) (void)window_set_avlist(win, (Window_attribute *)
	    			(LINT_CAST(attrs[1])));
	    break;

	  default:
	    if (ATTR_PKG_WIN == ATTR_PKG(attrs[0]))
		(void)fprintf(stderr,
			"window_set: Window attribute not allowed.\n%s\n",
			attr_sprint((char *)NULL, (unsigned)attrs[0]));
	    break;

	}
    }
    /* This unlock may operate on a different fd then the original lock
     * This is supposed to work.  See compatibility_info attr.
     */
    if (win->fd >= 0) (void)win_unlockdata(win->fd);
    
    if (adjusted && win->layout_proc)
	(win->layout_proc)(win->object, win->object, WIN_ADJUSTED);
    
    if (allow_registration && do_registration && win->well_behaved) {
	if (win->registered) (void)win_unregister(old_object);
	(void)win_register(win->object, win->pixwin, event_proc, destroy_proc,
		     (unsigned)notify_flags);
	win->registered = TRUE;
    } else if (win->well_behaved) {
	if (event_proc != window_default_event_func)
	    (void)notify_set_event_func(win->object, event_proc, NOTIFY_SAFE);
	if (destroy_proc != window_default_destroy_func)
	    (void)notify_set_destroy_func((Notify_client)(LINT_CAST(win->object)), destroy_proc);
    }
    return TRUE;
}


/* convert any row or column unit attribute values
 * in avlist to pixel units, using the window's row, column
 * parameters.
 */
Pkg_private void
window_rc_units_to_pixels(client_win, avlist)
	Window client_win;
	Window_attribute avlist[];
{
    register struct window *win;
    
    win = client_to_win(client_win);
    if (!win) return;

    attr_rc_units_to_pixels((char **)(LINT_CAST(avlist)), 
			    actual_column_width(win), actual_row_height(win), 
			    win->left_margin, win->top_margin,
			    win->column_gap, win->row_gap);
}


Private void
window_scan_and_convert_to_pixels(win, avlist)
	register struct window *win;
	Window_attribute avlist[];
{   
    register Window_attribute *attrs; 

    for (attrs = avlist; *attrs; attrs = window_attr_next(attrs)) {
	switch (attrs[0]) {

	  struct pixfont *pw_pfsysopen();
	  
	  case WIN_FONT:
	    win->font = (struct pixfont *)attrs[1];
	    if (!win->font) win->font = pw_pfsysopen();
	    break;

	  case WIN_TOP_MARGIN:
	    win->top_margin = (int)attrs[1];
	    break;
	    
	  case WIN_BOTTOM_MARGIN:
	    win->bottom_margin = (int)attrs[1];
	    break;
	    
	  case WIN_LEFT_MARGIN:
	    win->left_margin = (int)attrs[1];
	    break;
	    
	  case WIN_RIGHT_MARGIN:
	    win->right_margin = (int)attrs[1];
	    break;
	    
	  case WIN_ROW_HEIGHT:
	    win->row_height = (int)attrs[1];
	    break;
	    
	  case WIN_COLUMN_WIDTH:
	    win->column_width = (int)attrs[1];
	    break;
	    
	  case WIN_ROW_GAP:
	    win->row_gap = (int)attrs[1];
	    break;
	    
	  case WIN_COLUMN_GAP:
	    win->column_gap = (int)attrs[1];
	    break;
	    
	  case WIN_COMPATIBILITY_INFO:
	    win->fd = (int)attrs[1];
	    win->pixwin = (struct pixwin *)attrs[2];
	  /* FALL THRU */
	  case WIN_COMPATIBILITY:
	    win->well_behaved = FALSE;
	    break;
	    
	  case WIN_SHOW_UPDATES:
	    win->show_updates = (int)attrs[1];
	    break;
	    
	}
    }
    window_rc_units_to_pixels(win->object, avlist);
}


Private void
set_mask_bit(mask, value)
	register Inputmask *mask;
	Window_input_event value;
{   
int	i;

    switch (value) {

        case WIN_NO_EVENTS:
	    (void)input_imnull(mask);
	    break;
	    
        case WIN_MOUSE_BUTTONS:
	    win_setinputcodebit(mask, MS_LEFT);
	    win_setinputcodebit(mask, MS_MIDDLE);
	    win_setinputcodebit(mask, MS_RIGHT);
	    mask->im_flags |= IM_NEGEVENT;
	    break;
	    
        case WIN_LEFT_KEYS:
	    for(i = 1; i < 16; i++)
		    win_setinputcodebit(mask, KEY_LEFT(i));
	    break;

        case WIN_TOP_KEYS:
	    for(i = 1; i < 16; i++)
		    win_setinputcodebit(mask, KEY_TOP(i));
	    break;

        case WIN_RIGHT_KEYS:
	    for(i = 1; i < 16; i++)
		    win_setinputcodebit(mask, KEY_RIGHT(i));
	    break;

        case WIN_ASCII_EVENTS:
	    mask->im_flags |= IM_ASCII;
	    mask->im_flags |= IM_META;
	    break;
	    
        case WIN_UP_ASCII_EVENTS:
	    mask->im_flags |= IM_NEGASCII;
	    mask->im_flags |= IM_NEGMETA;
	    break;
	    
        case WIN_UP_EVENTS:
	    mask->im_flags |= IM_NEGEVENT;
	    break;
	    
        case WIN_IN_TRANSIT_EVENTS:
	    mask->im_flags |= IM_INTRANSIT;
	    break;
	    
        default:
	    win_setinputcodebit(mask, (int) value);
	    break;
    }   
}


Private void
unset_mask_bit(mask, value)
	register Inputmask *mask;
	Window_input_event value;
{   

    switch (value) {
        case WIN_NO_EVENTS:
	    (void)input_imnull(mask);
	    break;
	    
        case WIN_MOUSE_BUTTONS:
	    win_unsetinputcodebit(mask, MS_LEFT);
	    win_unsetinputcodebit(mask, MS_MIDDLE);
	    win_unsetinputcodebit(mask, MS_RIGHT);
	    mask->im_flags &= ~IM_NEGEVENT;
	    break;
	    
        case WIN_ASCII_EVENTS:
	    mask->im_flags &= ~IM_ASCII;
	    mask->im_flags &= ~IM_META;
	    break;
	    
        case WIN_UP_ASCII_EVENTS:
	    mask->im_flags &= ~IM_NEGASCII;
	    mask->im_flags &= ~IM_NEGMETA;
	    break;
	    
        case WIN_UP_EVENTS:
	    mask->im_flags &= ~IM_NEGEVENT;
	    break;
	    
        case WIN_IN_TRANSIT_EVENTS:
	    mask->im_flags &= ~IM_INTRANSIT;
	    break;
	    
        default:
	    win_unsetinputcodebit(mask, (int) value);
	    break;
    }   
}

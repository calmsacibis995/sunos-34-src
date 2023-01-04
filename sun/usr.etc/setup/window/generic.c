
#ifndef lint
static	char sccsid[] = "@(#)generic.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup.h"

static void	draw_status();
static void	generic_event();

/* assumes text items have a non-null notify string */
#define	IS_TEXT_ITEM(item)	(panel_get((item), PANEL_NOTIFY_STRING))

/* Get the choices for an item. */
void
get_choices(ws, config, item)
Workstation	ws;
Config_type	config;
Panel_item	item;
{
    register int	index;
    register char	*string;
    
    /* get rid of the old choices */
    panel_set(item, PANEL_CHOICE_STRINGS, "", 0, PANEL_PAINT, PANEL_NONE, 0);

    SETUP_FOREACH_CHOICE(ws, config, index, string)
        panel_set(item, PANEL_CHOICE_STRING, index, string, 
			PANEL_PAINT, PANEL_NONE, 0);
    SETUP_END_FOREACH
}


get_object_choices(object, object_attr, choice_attr, item)
Opaque		object;
Setup_attribute	object_attr, choice_attr;
Panel_item	item;
{
    register int	index;
    register Opaque	sub_object;
    
    /* get rid of the old choices */
    panel_set(item, PANEL_CHOICE_STRINGS, "", 0, PANEL_PAINT, PANEL_NONE, 0);

    SETUP_FOREACH_OBJECT(object, object_attr, index, sub_object)
        panel_set(item, 
	    PANEL_CHOICE_STRING, index, setup_get(sub_object, choice_attr),
			PANEL_PAINT, PANEL_NONE, 0);
    SETUP_END_FOREACH
}


Panel_setting
set_text_info(item, event)
Panel_item	item;
Event		*event;
{
    Object_info	*info	= (Object_info *) panel_get(item, PANEL_CLIENT_DATA);
    char	*value	= (char *) panel_get_value(item);

    setup_set(info->obj, info->attr, value, 0);
    return set_caret(item, event);
}


Panel_setting
set_caret(text_item, event)
Panel_item	text_item;
Event		*event;
{
    Panel	panel	= (Panel) panel_get(text_item, PANEL_PARENT_PANEL);
    Object_info	*info 	= (Object_info *) panel_get(text_item, PANEL_CLIENT_DATA);
    Panel_item	item;

	/* if this item still needs attention, leave caret here */
	
    if (info && setup_get(info->obj, SETUP_STATUS, info->attr)) { 
         panel_set(panel, PANEL_CARET_ITEM, text_item, 0);
         return (PANEL_NONE);
    } 
    	/* otherwise, leave it at the next text item still needing attention */
    	
    panel_each_item(panel, item)
        if (panel_get(item, PANEL_SHOW_ITEM) 	/* a displayed text item */
            && IS_TEXT_ITEM(item)		/*  with 'info'  */
            && (info = (Object_info *) panel_get(item, PANEL_CLIENT_DATA)))  
           {
            if (setup_get(info->obj, SETUP_STATUS, info->attr)) {
                panel_set(panel, PANEL_CARET_ITEM, item, 0);
                return (PANEL_NONE);		/* leave caret here */
            }   
        }
    panel_end_each; 
    
    	/* all are OK, just advance the caret to the next text item */
    if (event)
	return (panel_text_notify(text_item, event));
    else
	/* leave the caret where it is */
	return PANEL_NONE;
}




void
set_info(item, value, event)
Panel_item	item;
unsigned int	value;
Event		*event;
{
    Object_info	*info	= (Object_info *) panel_get(item, PANEL_CLIENT_DATA);

    setup_set(info->obj, info->attr, value, 0);
}


/* get the object info from the
 * middle end.
 */
void
get_object_info(panel)
Panel	panel;
{
    register Panel_item		item;
    register Object_info	*info;
    
    panel_each_item(panel, item)
	info = (Object_info *) panel_get(item, PANEL_CLIENT_DATA);
	if (info)
	    glue(item, info->obj);
    panel_end_each;
}


void
glue(item, object)
register Panel_item	item;
register Opaque		object;
{
    
    register Object_info *info = (Object_info *) panel_get(item, PANEL_CLIENT_DATA);
    
    /* nothing to glue to if no object info */
    if (!info)
	return;

    /* unglue if differnet object */
    if (info->obj && info->obj != object)
	setup_set(info->obj, SETUP_OPAQUE, info->attr, 0, 0);

    /* glue the item to the
     * object.
     */
    info->obj = object;

    /* nothing more to do if no
     * new object to glue to.
     */
    if (!object)
	return;

    /* glue the object to the item,
     * if the item does not represent the
     * "NONE" attribute of the object.
     */
    if (info->attr != SETUP_NONE) {
	setup_set(object, SETUP_OPAQUE, info->attr, item, 0);
	glue_value(item, object, info->attr, 
	    (info->attr == SETUP_ALL) ? 0 : setup_get(object, info->attr));
    }
}


glue_value(item, object, attr, value)
Panel_item	item;
Opaque		object;
Setup_attribute	attr;
Opaque		value;
{
    switch (attr) {
	case SETUP_NONE:
	    return;

	case SETUP_ALL:
	    break;

	case DISK_SIZE_STRING_LEFT:
	case DISK_FREE_SPACE_STRING_LEFT:
	case HARD_LETTER:
	case HARD_WHAT_IT_IS:
	case HARD_SIZE_STRING:
	    panel_set(item, PANEL_LABEL_STRING, value, 
		PANEL_PAINT, PANEL_NONE, 0);
	    break;
	    
	default:
	    panel_set(item, PANEL_VALUE, value, PANEL_PAINT, PANEL_NONE, 0);
	    break;
    }
    draw_status(item, setup_get(object, SETUP_STATUS, attr));
    panel_paint(item, PANEL_CLEAR);

    /* now update any child items */
    switch (attr) {
  	case WS_TYPE:
  	    update_ws_type(item, value);
  	    break;

  	case WS_TAPE_LOC:
  	    update_tape_location(item, value);
  	    break;

  	case WS_ETHERTYPE:
  	    update_e_board_type(item, value);
  	    break;

  	case WS_YPTYPE:
  	    update_yp_type(item, value);
  	    break;

	case PARAM_DISK_DISPLAY_UNITS:
	    update_units(object);
	    break;

        case DISK_PARAM_OVERLAPPING:
        case DISK_PARAM_FLOATING:
            update_disk_overlap(object);
	    break;

	case HARD_TYPE:
	case HARD_WHAT_IT_IS:
	    update_hp_type(object);
	    break;

	case HARD_OFFSET_STRING:
	case HARD_OFFSET_STRING_LEFT:
	    update_hp_offset(object);
	    break;

	case HARD_SIZE_STRING:
	case HARD_SIZE_STRING_LEFT:
	    update_hp_size(object);
	    break;

        case HARD_MIN_SIZE_IN_UNITS:
        case HARD_MAX_SIZE_IN_UNITS:
	    update_hp_size_slider(object);
	    break;
	   
	case HARD_PARAM_FREEHOG:
	    update_hp_image(object);
	    break;

  	case PARAM_AUTOHOST:
  	    update_autohost(item, value);
  	    break;

       default:
	   break;
    }
}


Object_info *
object_info_plus(obj, attr, other)
Opaque	obj;
Setup_attribute	attr;
Opaque	other;
{
    Object_info		*info = new(Object_info);

    info->obj	= obj;
    info->attr	= attr;
    info->other	= other;

    return info;
}


void
handle_error(obj, attr, display_value, err_msg)
Opaque		obj;
Setup_attribute	attr;
caddr_t		display_value;
char		*err_msg;
{
    Panel_item	item = (Panel_item) setup_get(obj, SETUP_OPAQUE, attr);

    glue_value(item, obj, attr, display_value);
    message_print(err_msg);
}


/* make a panel with dimensions of rect */
Panel
make_panel(rect)
Rect	rect;
{
    Panel	panel = 0;
	
    panel = panel_begin(tool,
    			PANEL_FONT, text_font,
    			PANEL_LABEL_BOLD, TRUE,
			PANEL_ITEM_Y_GAP, 11,
			PANEL_EVENT_PROC, generic_event,
			0);
    if (!panel) {
        setup_error("make_panel: unable to create another panel\n");
        return;
    }			
    win_setrect(win_get_fd(panel), &rect);
    show_panel(panel, FALSE);
    return(panel);                           
}


/* coerce a carriage-return on the next non-ascii
 * event after a text item has been modified 
 */
static void
generic_event(item, event)
register Panel_item	item;
register Event		*event;
{
    register short	commit = FALSE;
    static Panel_item	modified_item;
    register short	same_item = (modified_item == item);
    register Event	new_event;
    
    if (modified_item && !same_item && event_is_down(event)) 
        switch (event_id(event)) {
            case MS_LEFT: 
            case MS_RIGHT: 
    	    	new_event = *event;
    	    	event_set_id(&new_event, '\n');
    	    	panel_default_handle_event(modified_item, &new_event);
            	modified_item = 0;
                break;
            default:
                break;    
	}           
    
    panel_default_handle_event(item, event);
    	
    if (event_is_ascii(event))
        switch(event_id(event)) {
            case '\n':
            case '\r':
            case '\t':
                if (same_item)
                    modified_item = 0;
                break;
            default:
    		modified_item = item;
                break;
         }       
}


/* show panel if show_it is TRUE */
void
show_panel(panel, show_it)
Panel	panel;
short	show_it;
{
    int		panel_fd	= win_get_fd(panel);

    if (show_it) {
	get_object_info(panel);
	win_insert(panel_fd);
    } else
	win_remove(panel_fd);
}


static void
draw_status(item, status)
Panel_item	item;
int		status;
{
    Pixrect	*label	= (Pixrect *) panel_get(item, PANEL_LABEL_IMAGE);
    Object_info	*info	= (Object_info *) panel_get(item, PANEL_CLIENT_DATA);
    int		op	= status ? PIX_SRC : PIX_CLR;

    if (!label)
	return;

    image_box(label, op);
    /*
    panel_paint(item, PANEL_NO_CLEAR);
    */
}

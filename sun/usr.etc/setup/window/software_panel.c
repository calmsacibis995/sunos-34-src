
#ifndef lint
static	char sccsid[] = "@(#)software_panel.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup.h"

static Panel		panel 	= 0;
static Panel_item	arch_item, oswg_item,
			all_item, clear_item, default_item;

static void		init_software_screen();
static void		get_arch_item_choices();
static void		show_choices();
static void		button_selected();
 
/* Show the software screen */
void
software_show(unused, show_it)
Opaque	unused;
short	show_it;
{
    
    if (!panel) 
        init_software_screen();
       
    if (show_it) {
	get_object_choices(ws, WS_ARCH_SERVED_ARRAY, 
			       ARCH_SERVED_NAME, arch_item);
	show_choices(arch_item, 0);
    }

    show_panel(panel, show_it);
}

/* Initialize the software screen */
static void
init_software_screen()
{
    int 	center = display_rect.r_width / 2 - 200;
    int 	value_offset = 235;
                                        
    panel = make_panel(display_rect);

/* architecture cycle choice item */    
    arch_item = 
        panel_create_item(panel, PANEL_CHOICE,
                          PANEL_MENU_TITLE_STRING, "OPTIONAL SOFTWARE FOR :",
    		          PANEL_LABEL_IMAGE, 
    		          	image_string("OPTIONAL SOFTWARE FOR :"),
                          CYCLE_ATTRS(center, ATTR_ROW(1), value_offset),
                          PANEL_NOTIFY_PROC, show_choices,
    		          PANEL_SHOW_ITEM, TRUE,
    		          0);
    		          
/* checklist of software packages */    
    oswg_item = 
        panel_create_item(panel, PANEL_TOGGLE,
    		          PANEL_LABEL_X, center,
    		          PANEL_LABEL_Y, ATTR_ROW(3),
    		          PANEL_LAYOUT, PANEL_VERTICAL,
    		          PANEL_FEEDBACK, PANEL_MARKED,
			  PANEL_CLIENT_DATA, object_info(0, ARCH_OSWG),
     		          PANEL_NOTIFY_PROC, set_info,
   		          PANEL_SHOW_ITEM, TRUE,
    		          0);
    		          

/* buttons */

    clear_item = 
        panel_create_item(panel, PANEL_BUTTON,
                          PANEL_LABEL_X, center - 200,
                          PANEL_LABEL_Y, ATTR_ROW(3),
                          PANEL_LABEL_IMAGE, 
                              panel_button_image(panel, "Clear", 0, image_font),
                          PANEL_CLIENT_DATA, object_info(0, ARCH_OSWG_CLEAR),
                          PANEL_NOTIFY_PROC, button_selected,
                          0 );

    all_item =
        panel_create_item(panel, PANEL_BUTTON,
                          PANEL_LABEL_X, center - 200,
                          PANEL_LABEL_Y, ATTR_ROW(4),
                          PANEL_LABEL_IMAGE, 
                              panel_button_image(panel, "All", 0, image_font),
                          PANEL_CLIENT_DATA, object_info(0, ARCH_OSWG_ALL),
                          PANEL_NOTIFY_PROC, button_selected,
                          0 );

    default_item = 
        panel_create_item(panel, PANEL_BUTTON,
                          PANEL_LABEL_X, center - 200,
                          PANEL_LABEL_Y, ATTR_ROW(5),
                          PANEL_LABEL_IMAGE, 
                              panel_button_image(panel, "Common Choices", 0, image_font),
                          PANEL_CLIENT_DATA, object_info(0, ARCH_OSWG_DEFAULT),
                          PANEL_NOTIFY_PROC, button_selected,
                          0 );

}


static void
show_choices(item, value, event)
Panel_item	item;
int		value;
Event		*event;
{
    Arch_served	arch; 
    Oswg	oswg; 
    int		arch_type, index;

    arch = setup_get(ws, WS_ARCH_SERVED_ARRAY, value);
    glue(oswg_item, arch);
    setup_set(arch, SETUP_CALLBACK, handle_error, 0);

  /*  
    get_object_choices(ws, WS_OSWG, OSWG_DESCRIPTION, oswg_item);
   */
   
    arch_type = (int) setup_get(arch, ARCH_SERVED_TYPE);
   
    /* get rid of the old choices */
    panel_set(oswg_item, PANEL_CHOICE_STRINGS, "", 0, 
    			 PANEL_PAINT, PANEL_NONE, 
    			 0);
 
    SETUP_FOREACH_OBJECT(ws, WS_OSWG, index, oswg)
        panel_set(oswg_item,
            PANEL_CHOICE_STRING, index, 
                setup_get(oswg, OSWG_DESCRIPTION, arch_type),
            PANEL_PAINT, PANEL_NONE, 
            0);
    SETUP_END_FOREACH
}


static void
button_selected(item, event)
Panel_item	item;
Event		*event;
{
    Object_info 	*info;
    Arch_served		arch; 
    Setup_attribute	attr; 

    info = (Object_info *) panel_get(oswg_item, PANEL_CLIENT_DATA);
    arch = info->obj;
    info = (Object_info *) panel_get(item, PANEL_CLIENT_DATA);
    attr = info->attr;

    setup_set(arch, attr, 0);
}

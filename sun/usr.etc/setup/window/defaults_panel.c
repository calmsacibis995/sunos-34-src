
#ifndef lint
static	char sccsid[] = "@(#)defaults_panel.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup.h"

static Panel		panel 	= 0;
static Panel_item	beginhost;

static void		init_defaults_screen();
 
/* Show the defaults screen */
void
defaults_show(unused, show_it)
Opaque	unused;
short	show_it;
{
    if (!panel) 
        init_defaults_screen();
       
    show_panel(panel, show_it);
}

/* Initialize the defaults screen */
static void
init_defaults_screen()
{
    Panel_item	display_units, autohost, sendmail, preserve;
    int 	center = display_rect.r_width / 2 - 200;
    int 	value_offset = 210;
    int 	indent = 20;
    int 	y_delta = 4;
                                        
    panel = make_panel(display_rect);
    
    panel_create_item(panel, PANEL_TEXT,
                      PANEL_LABEL_X, center,
                      PANEL_LABEL_Y, ATTR_ROW(2),
                      PANEL_LABEL_IMAGE, image_string("Network Number:"),
                      PANEL_VALUE_X, center + value_offset,
                      PANEL_VALUE_Y, ATTR_ROW(2) + y_delta,
                      PANEL_VALUE_DISPLAY_LENGTH, 15,
                      PANEL_CLIENT_DATA, object_info(ws, WS_NETWORK),
                      PANEL_NOTIFY_PROC, set_text_info,
                      0);

    autohost =                       
        panel_create_item(panel, PANEL_CHOICE,
		      CYCLE_ATTRS(center, ATTR_ROW(3), value_offset),
                      PANEL_LABEL_IMAGE, image_string("Auto Host Numbering:"),
                      PANEL_MENU_TITLE_STRING, "Auto Host Numbering:",
                      PANEL_CHOICE_STRINGS, "No", "Yes", 0,
                      PANEL_CLIENT_DATA, object_info(ws, PARAM_AUTOHOST),
    		      PANEL_NOTIFY_PROC, set_info,
    		      0);
    panel_set(autohost, PANEL_VALUE_X, center + value_offset,
    			PANEL_VALUE_Y, ATTR_ROW(3), 
    			0);

    beginhost =     		      
        panel_create_item(panel, PANEL_TEXT,
    		      PANEL_LABEL_X, center + indent,
    		      PANEL_LABEL_Y, ATTR_ROW(4),
    		      PANEL_LABEL_IMAGE, image_string("Begin Numbering at:"),
                      PANEL_VALUE_DISPLAY_LENGTH, 15,
                      PANEL_CLIENT_DATA, 
                      		object_info(ws, PARAM_FIRSTHOST_STRING_LEFT), 
                      PANEL_NOTIFY_PROC, set_text_info,
                      PANEL_SHOW_ITEM, 
                              	(int) setup_get(ws, PARAM_AUTOHOST) == 1,
                      0);
    panel_set(beginhost, PANEL_VALUE_X, center + value_offset,
    			PANEL_VALUE_Y, ATTR_ROW(4) + y_delta, 
    			0);
    
    
    display_units =
        panel_create_item(panel, PANEL_CHOICE,
		      CYCLE_ATTRS(center, ATTR_ROW(5), value_offset),
                      PANEL_LABEL_IMAGE, image_string("Display Units:"),
                      PANEL_MENU_TITLE_STRING, "Display Units:",
                      PANEL_CLIENT_DATA,
                          object_info(ws, PARAM_DISK_DISPLAY_UNITS),
                      PANEL_NOTIFY_PROC, set_info,
                      0);
                      
    get_choices(ws, CONFIG_DISK_DISPLAY_UNITS, display_units);
    panel_set(display_units, PANEL_VALUE_X, center + value_offset,
    			     PANEL_VALUE_Y, ATTR_ROW(5), 
    			     0);
                     
    sendmail =
        panel_create_item(panel, PANEL_CHOICE,
		      CYCLE_ATTRS(center, ATTR_ROW(6), value_offset),
                      PANEL_LABEL_IMAGE, image_string("Mail Configuration:"),
                      PANEL_MENU_TITLE_STRING, "Mail Configuration:",
                      PANEL_CLIENT_DATA, object_info(ws, WS_MAILTYPE),
                      PANEL_NOTIFY_PROC, set_info,
                      0);
                      
    get_choices(ws, CONFIG_MAIL_TYPES, sendmail);
    panel_set(sendmail, PANEL_VALUE_X, center + value_offset,
    			PANEL_VALUE_Y, ATTR_ROW(6), 
    			0);

    preserve =                                               
        panel_create_item(panel, PANEL_CHOICE,
		      CYCLE_ATTRS(center, ATTR_ROW(7), value_offset),
                      PANEL_LABEL_IMAGE, image_string("Preserve Disk State:"),
                      PANEL_MENU_TITLE_STRING, "Preserve Disk State:",
                      PANEL_CHOICE_STRINGS, "No", "Yes", 0,
                      PANEL_CLIENT_DATA, object_info(ws, WS_PRESERVED),
                      PANEL_NOTIFY_PROC, set_info,
                      0);
    panel_set(preserve, PANEL_VALUE_X, center + value_offset,
    			PANEL_VALUE_Y, ATTR_ROW(7), 
    			0);
}


void
update_autohost(item, value)
Panel_item      item;
int             value;
{
    int show_it = value == 1;

    panel_set(beginhost, PANEL_SHOW_ITEM, show_it, 0);
    if (show_it)
        set_caret(item, 0);
}

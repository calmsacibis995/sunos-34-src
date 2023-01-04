#ifndef lint
static	char sccsid[] = "@(#)tty_defaults.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
* Copyright (c) 1985 by Sun Microsystems, Inc.
*/


#include "tty_global.h"
#include "tty_item.h"


static	Item	net_number_item;
static	Item	auto_host_number_item;
static	Item		first_host_item;
static	Item	display_units_item;
static	Item	cylinder_rounding_item;
static	Item	mail_type_item;
static	Item	preserve_homedirs_item;

extern	Workstation 	ws;
extern	Form		defaults_form;

tty_defaults_init()
{   
    
    net_number_item = 
	TEXT_ITEM(defaults_form, 1, 0, "Network Number:", 27, 20);
    glue(net_number_item, ws, WS_NETWORK, (Item)NULL);
    form_set(defaults_form, FORM_CURRENT_IO_ITEM, net_number_item, 0);
    
    auto_host_number_item = 
	CHOICE_ITEM(defaults_form, 2, 0, "Auto Host Numbering:", 27);
    tty_yes_no_choice(auto_host_number_item);
    first_host_item =
	TEXT_ITEM(defaults_form, 3, 4, "Begin Numbering at:", 27, 20);
    glue(first_host_item, ws, PARAM_FIRSTHOST_STRING_LEFT, (Item)NULL);
    glue(auto_host_number_item, ws, PARAM_AUTOHOST, first_host_item);
    form_item_set(first_host_item, 
		  ITEM_DISPLAYED, 	(int)setup_get(ws, PARAM_AUTOHOST), 
		  0);
    
    display_units_item = 
	CHOICE_ITEM(defaults_form, 4, 0, "Display Units:", 27);
    glue_choices(display_units_item, ws, CONFIG_DISK_DISPLAY_UNITS);
    glue(display_units_item, ws, PARAM_DISK_DISPLAY_UNITS, (Item)NULL);
/*     form_item_set(display_units_item, ITEM_CHOICE_CYCLE_DISPLAY, TRUE, 0); */

    
    mail_type_item = 
	CHOICE_ITEM(defaults_form, 5, 0, "Mail Configuration:", 27);
    glue_choices(mail_type_item, ws, CONFIG_MAIL_TYPES);
    glue(mail_type_item, ws, WS_MAILTYPE, (Item)NULL); 
    
    
    preserve_homedirs_item = 
	CHOICE_ITEM(defaults_form, 6, 0, "Preserve Disk State:", 27);
    tty_yes_no_choice(preserve_homedirs_item);
    glue(preserve_homedirs_item, ws, WS_PRESERVED, (Item)NULL);    
    
}



tty_yes_no_choice(item)
Item	item;
{   
    form_item_set(item, ITEM_CHOICE_RESET, 0);
    
    form_item_set(item, 
		  ITEM_CHOICE_STRING, 	"No", 
		  ITEM_CHOICE_STRING, 	"Yes",
		  0);
}
    

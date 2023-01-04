#ifndef lint
static	char sccsid[] = "@(#)tty_glue.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "tty_global.h"

int	callback_error;	/* global callback status  */

/* 
 * Routines that glue tty_ application, Form package and setup
 * together.
 */

typedef struct {
    caddr_t		setup_obj;	/* a  setup object */
    Setup_attribute	attr;		/* its attribute */
    Item		item;		/* item to display if value is true */
} Glue_info;

typedef Glue_info	*Glue;


glue(item, obj, attr, disp_item)
Item		item;
caddr_t		obj;
Setup_attribute	attr;
Item		disp_item;
{   
    Glue	glue;
    
    glue = (Glue) form_item_get(item, ITEM_CLIENT_DATA);
    if (glue == (Glue)NULL)
	glue = (Glue) malloc(sizeof(Glue_info));
    glue->setup_obj = obj;
    glue->attr = attr;
    glue->item = disp_item;
    form_item_set(item, 
		  ITEM_CLIENT_DATA, glue, 
		  ITEM_NOTIFY_PROC, glue_notify_proc, 
		  0);
    setup_set(obj, SETUP_OPAQUE, attr, item,  0);
    
    form_item_set(item, ITEM_HILIGHTED, setup_get(obj, SETUP_STATUS, attr), 
		  0);
    if (attr != SETUP_ALL) {
	form_item_set(item, ITEM_VALUE, setup_get(obj, attr), 0);
    }
}




glue_choices(item, ws, config)
Item		item;
Workstation	ws;
Config_type	config;
{   
    int		index;
    char	*str;
    
    SETUP_FOREACH_CHOICE(ws, config, index, str)
	form_item_set(item, ITEM_CHOICE_STRING, str, 0);
    SETUP_END_FOREACH
    
}


glue_toggle_strings(item, ws, config)
Item		item;
Workstation	ws;
Config_type	config;
{   
    int		index;
    char	*str;
    
    SETUP_FOREACH_CHOICE(ws, config, index, str)
	form_item_set(item, ITEM_TOGGLE_STRING, str, 0);
    SETUP_END_FOREACH
    
}



int
glue_notify_proc(item, ch)
Item	item;
int	ch;
{   
    register	Glue	glue;
    
    glue = (Glue) form_item_get(item, ITEM_CLIENT_DATA);
    
    if (glue == (Glue)NULL)
	return(FALSE);
    
    setup_set(glue->setup_obj, glue->attr, 
	      form_item_get(item, ITEM_VALUE), 0);
    
    if (glue->item) {	
	/* display another item based upon the state of the picked item */
	form_item_set(glue->item, ITEM_DISPLAYED, 
		      (int) form_item_get(item, ITEM_VALUE), 0);
	form_item_display(glue->item);
	form_set((Form)form_item_get(item, ITEM_FORM), FORM_REFRESH, 0);	
	glue_notify_proc(glue->item, NULL);
    }
    
    return(FALSE);
}




void
glue_callback_proc(obj, attr, display_value, err_msg)
Opaque          obj;
Setup_attribute attr;
caddr_t         display_value;
char            *err_msg;
{
    register	Item		item;
    extern	Cursor_control	cursor_control;
    register	int		status;
    
    item = setup_get(obj, SETUP_OPAQUE, attr);
    if (item == (Item)NULL) {
	return;
    }
    
    /* use err_msg to determine if there is an error */
    if (err_msg != NULL) {
	callback_error = TRUE;
	tty_error_msg(err_msg);
	cursor_control = CURSOR_CONTROL_STAY_PUT;
    }
    else {
	callback_error = FALSE;
    }
    /*  status == TRUE  means there is an error  */
    status = (int)setup_get(obj, SETUP_STATUS, attr);
    
    form_item_set(item, ITEM_HILIGHTED, status, 0);
    form_item_set(item, ITEM_VALUE, display_value, 0);
    
    form_item_display(item);
    form_set((Form)form_item_get(item, ITEM_FORM), FORM_REFRESH, 0);
}
	
    

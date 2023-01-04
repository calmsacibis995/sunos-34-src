#ifndef lint
static	char sccsid[] = "@(#)form_button.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "form.h"
#include "object.h"
#include "form_attr.h"
#include "form_match.h"

typedef struct {
    Object	*parent;
    
} Button_info;

static	void	button_set();
static	caddr_t	button_get();
static	void	button_destroy();
static	void	button_display();
static	int	button_process_input();



Item
button_create(parent)
Object	*parent;
{   
    Button_info		*button;
    Object		*obj;
    
    button = (Button_info *) malloc(sizeof(Button_info));
    
    button->parent = parent;
    
    obj = (Object *) malloc(sizeof(Object));
    
    obj->data 		= (caddr_t) button;
    obj->set 		= button_set;
    obj->get 		= button_get;
    obj->destroy 	= button_destroy;
    obj->display 	= button_display;
    obj->process_input 	= button_process_input;
    
    return((Item)obj);
}



static
void
button_set(obj, avlist)
Object	*obj;
register	Form_avlist	avlist;
{   
    register	Button_info	*button;
    register	Form_attribute	attr;
    		WINDOW		*win;
    		int		row, col;
    
    button = (Button_info *) obj->data;
    
    while (attr = (Form_attribute) *avlist++) {
	switch (attr) {
	  case ITEM_SET_CURSOR:
	    if (item_label_calculate_display(button->parent, 
					     &win, &row, &col)) {
		wmove(win, row, col);
		wrefresh(win);
	    }
	    break;
	    
	}
    }
}



    
static
caddr_t
button_get(obj, attr)
Object	*obj;
Form_attribute	attr;
{   
    register	Button_info	*button;
    
    button = (Button_info *) obj->data;
    
    switch (attr) {

    }
}


   

static
void
button_destroy(obj)
Object	*obj;
{   
    free(obj->data);
    free(obj);
}
    


static
void
button_display(obj)
Object	*obj;
{   
    register	Button_info	*button;
    
    button = (Button_info *) obj->data;
    
}


static
int
button_process_input(obj, info)
Object	*obj;
Input_info	*info;
{   
    register	Button_info	*button;
    
    button = (Button_info *) obj->data;
    
}


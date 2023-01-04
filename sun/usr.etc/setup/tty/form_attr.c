#ifndef lint
static	char sccsid[] = "@(#)form_attr.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "form.h"
#include "object.h"
#include "form_attr.h"

int
form_notify_proc(item, ch)
Item	item;
int	ch;
{
    return (FALSE);
}



Item
form_item_create(form, item_type, display_type, args)
Form		*form;
caddr_t		item_type;
Form_attribute	display_type;
Form_attribute	args;
{   
    Object		*item, *item_create();
    Form_attribute	a[ATTR_STANDARD_SIZE];
    
    item = item_create(item_type);
    (*item->set)(item, attr_make(a, ATTR_STANDARD_SIZE, &args));
    
    form_set(form, 
	     display_type, item, 
	    0);
    form_item_set(item, ITEM_FORM, form, 0);
    
    return((Item)item);
}



void
form_item_set(item, args)
Object		*item;
Form_attribute	args;
{   
    Form_attribute	a[ATTR_STANDARD_SIZE];

    (*item->set)(item, attr_make(a, ATTR_STANDARD_SIZE, &args));
}

caddr_t
form_item_get(item, attr, op1, op2)
Object		*item;
Form_attribute	attr;
caddr_t		op1;
caddr_t		op2;
{   
    
    (*item->get)(item, attr, op1, op2);
}



void
form_item_display(obj)
Object	*obj;
{   
    if ((int)form_get((Form)form_item_get(obj, ITEM_FORM), FORM_DISPLAY))
	(*obj->display)(obj);
}


form_item_process_input(obj, ch)
Object	*obj;
int	ch;
{   
    return((*obj->process_input)(obj, ch));
}

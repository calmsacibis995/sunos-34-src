#ifndef lint
static	char sccsid[] = "@(#)form.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */



#include "form.h"
#include "form_attr.h"

static	void	next_item();
static	void	prev_item();

/* 
* define a form --
*   a collection of items; some items may not receive input and are called
*   output only; the other types of items are input/output.
* 
*/


typedef struct ITEM_LIST {
    Item		item;
    struct ITEM_LIST	*next;
    struct ITEM_LIST	*prev;
} Item_list;


typedef struct {
    WINDOW	*win;			/* curses window the form uses   */
    int		first_row;		/* starting row number of items  */
    int		last_row;		/* last row number of items      */
    Item_list	*oo_head;		/* list of output only items	 */
    Item_list	*oo_tail;
    Item_list	*io_head;		/* list of input output items	 */
    Item_list	*io_tail;		
    Item_list	*cur_io_item;		/* current input output item	 */
    int		min_row, max_row;	/* min and max value of item rows */
    int		displayed;		/* TRUE if the form is displayed */
    Void_func	disp_func;		/* function to call after form 
					 * has been displayed.
					 */
} Form_info;



Form 
form_create(win, first_row, last_row)
WINDOW	*win;
int	first_row;
int	last_row;
{   
    register	Form_info	*new_form;
    
    
    new_form = (Form_info *) malloc(sizeof(Form_info));
    
    new_form->oo_head = (Item_list *) malloc(sizeof(Item_list));
    
    new_form->io_head = (Item_list *) malloc(sizeof(Item_list));
    
    new_form->oo_head->item = (Item)NULL;
    new_form->io_head->item = (Item)NULL;
    
    new_form->oo_head->next = (Item_list *)NULL;
    new_form->io_head->next = (Item_list *)NULL;
    new_form->oo_head->prev = (Item_list *)NULL;
    new_form->io_head->prev = (Item_list *)NULL;
    
    new_form->oo_tail = new_form->oo_head;
    new_form->io_tail = new_form->io_head;
    
    new_form->win = win;
    new_form->first_row = first_row;
    new_form->last_row = last_row;
    new_form->min_row = 1000000;
    new_form->max_row = -1000000;
    
    new_form->displayed = FALSE;
    new_form->disp_func = NULL;
    
    new_form->cur_io_item = (Item_list *)NULL;
    
    return((Form)new_form);
}



static
form_add_item(form, tail, item)
Form_info	*form;
Item_list	**tail;
Item		item;
{   
    register	int	row;
    
    (*tail)->next = (Item_list *) malloc(sizeof(Item_list));
    
    (*tail)->next->item = item;
    (*tail)->next->prev = (*tail);
    (*tail)->next->next = (Item_list *)NULL;
    *tail = (*tail)->next;
    form_item_set(item, 
		  ITEM_FORM, form, 
		 0);
    row = (int) form_item_get(item, ITEM_ROW);
    if (row < form->min_row)
	form->min_row = row;
    if (row > form->max_row)
	form->max_row = row;
}
    


form_display(form)
Form_info	*form;
{   
    register  Item_list	*lp;
    
    touchwin(form->win);
    wclear(form->win);
    
    for (lp = form->oo_head->next; lp != (Item_list *)NULL; lp = lp->next)
	form_item_display(lp->item);
    
    for (lp = form->io_head->next; lp != (Item_list *)NULL; lp = lp->next)
	form_item_display(lp->item);
    if (form->cur_io_item)
	form_item_set(form->cur_io_item->item, ITEM_SET_CURSOR, 0);
    
    if (form->disp_func != NULL)
	(*(form->disp_func))(form);
    
    wrefresh(form->win);
}




form_goto_item(form, dir)
Form_info	*form;
Item_direction	dir;
{   
    int		item_row, item_col;    
    int		t;
    
    do {
	if (dir == FORM_NEXT) {
	    next_item(form);
	}
	else {
	    prev_item(form);
	}
    } while (!(int)form_item_get(form->cur_io_item->item, ITEM_DISPLAYED));
    
    item_row = (int) form_item_get(form->cur_io_item->item, ITEM_ROW);
    item_col = (int) form_item_get(form->cur_io_item->item, ITEM_COL);
    
    if ((item_row >= form->first_row) && (item_row <= form->last_row)) {
	/* item is visble */
	form_item_set(form->cur_io_item->item, ITEM_SET_CURSOR, 0);
    }
    else {
	/* item is not visible, so scroll the screen */
	t = form->last_row - form->first_row;
	if (item_row < form->first_row) {
	    form->first_row = item_row;
	    form->last_row  = item_row + t;
	}
	else {
	    form->last_row = item_row;
	    form->first_row = item_row - t;
	}
	form_display(form);
	form_item_set(form->cur_io_item->item, ITEM_SET_CURSOR, 0);
    }
}

static
void
next_item(form)
Form_info	*form;
{
    int		item_type;
    caddr_t	sub_item;

    item_type = (int) form_item_get(form->cur_io_item->item, ITEM_TYPE);
    if (item_type == (int) FORM_ITEM_TOGGLE) {
	sub_item = form_item_get(form->cur_io_item->item, ITEM_SUB_ITEM);
	if (next_toggle(sub_item)) {
	    return;
	} 
    } else if (item_type == (int) FORM_ITEM_CHOICE) {
	sub_item = form_item_get(form->cur_io_item->item, ITEM_SUB_ITEM);
	if (next_choice(sub_item)) {
	    return;
	} 
    }
    form->cur_io_item = form->cur_io_item->next;
    if (form->cur_io_item == (Item_list *)NULL) {
        form->cur_io_item = form->io_head->next;
    }
}

static
void
prev_item(form)
Form_info	*form;
{
    int		item_type;
    caddr_t	sub_item;

    item_type = (int) form_item_get(form->cur_io_item->item, ITEM_TYPE);
    if (item_type == (int) FORM_ITEM_TOGGLE) {
	sub_item = form_item_get(form->cur_io_item->item, ITEM_SUB_ITEM);
	if (prev_toggle(sub_item)) {
	    return;
	} 
    } else if (item_type == (int) FORM_ITEM_CHOICE) {
	sub_item = form_item_get(form->cur_io_item->item, ITEM_SUB_ITEM);
	if (prev_choice(sub_item)) {
	    return;
	} 
    }
    form->cur_io_item = form->cur_io_item->prev;
    if (form->cur_io_item == form->io_head) {
	form->cur_io_item = form->io_tail;
    }
    item_type = (int) form_item_get(form->cur_io_item->item, ITEM_TYPE);
    if (item_type == (int) FORM_ITEM_TOGGLE) {
	sub_item = form_item_get(form->cur_io_item->item, ITEM_SUB_ITEM);
	last_toggle(sub_item);
    } else if (item_type == (int) FORM_ITEM_CHOICE) {
	sub_item = form_item_get(form->cur_io_item->item, ITEM_SUB_ITEM);
	last_choice(sub_item);
    }
}


/* 
 * Set the first item in the form; the item must be visible!!
 */

static
form_set_current_io_item(form, item)
Form_info	*form;
Item		item;
{   
   
    register  	Item_list	*lp;
    register	int		item_col, item_row;
    register	int		t;
    
    for (lp = form->io_head->next; lp != (Item_list *)NULL; lp = lp->next) {
	if (lp->item == item) {
	    form->cur_io_item = lp;
	
	    item_row = (int) form_item_get(form->cur_io_item->item, ITEM_ROW);
	    item_col = (int) form_item_get(form->cur_io_item->item, ITEM_COL);
	    
	    if ((item_row < form->first_row) || (item_row > form->last_row)) {
		/* item is not visible, so scroll the screen */
		t = form->last_row - form->first_row;
		if (item_row < form->first_row) {
		    form->first_row = item_row;
		    form->last_row  = item_row + t;
		}
		else {
		    form->last_row = item_row;
		    form->first_row = item_row - t;
		}
	    }
	    return;
	}
    }
}


static
int
display_hack(form)
register Form_info	*form;
{   
    register	int	size;
    register	int	range;
    
    size = form->last_row - form->first_row;
    range = form->max_row - form->min_row;
    if (range <= size) {
	form->first_row = form->min_row;
	form->last_row = form->first_row + size;
    }
    else {
	form->last_row = form->max_row;
	form->first_row = form->last_row - size;
    }
}



form_set(form, args)
Form_info	*form;
Form_attribute	args;
{   
    Item		item;
    Form_attribute	a[ATTR_STANDARD_SIZE], attr;
    Form_avlist		avlist;
    register int	diff;
    
    avlist = attr_make(a, ATTR_STANDARD_SIZE, &args);
    
    while (attr = (Form_attribute) *avlist++) {
	switch (attr) {
	  case FORM_INPUT_OUTPUT_ITEM:
	    form_add_item(form, &(form->io_tail), (Item) *avlist++);
	    break;
	    
	  case FORM_OUTPUT_ONLY_ITEM:
	    form_add_item(form, &(form->oo_tail), (Item) *avlist++);
	    break;
	    
	  case FORM_CURRENT_IO_ITEM:
	    form_set_current_io_item(form, (Item) *avlist++);
	    break;
	    
	  case FORM_CURSES_WINDOW:    
	    form->win = (WINDOW *) *avlist++;
	    break;
	    
	  case FORM_FIRST_ROW:
	    diff = form->last_row - form->first_row;
	    form->first_row = (int) *avlist++;
	    form->last_row = form->first_row + diff;
	    break;
	    
	  case FORM_LAST_ROW:
	    diff = form->last_row - form->first_row;
	    form->last_row = (int) *avlist++;
	    form->first_row = form->last_row - diff;
	    break;
	    
	  case FORM_REFRESH:
	    wrefresh(form->win);
	    break;
	    
	  case FORM_DISPLAY:
	    form->displayed = (int) *avlist++;
	    if (form->displayed)
		form_display(form);
	    break;
	    
	  case FORM_WIN_SIZE:
	    diff = (int) *avlist++;
	    form->last_row = form->first_row + (diff - 1);
	    break;
	    
	  case FORM_DISPLAY_FUNC:
	    form->disp_func = (Void_func) *avlist++;
	    break;
	    
	  case FORM_DISPLAY_HACK:
	    display_hack(form);
	    break;
	  
	  default:
	    avlist = attr_skip(attr, avlist);
	    break;
	}
    }
}
	    



caddr_t
form_get(form, attr, op1, op2)
Form_info	*form;
Form_attribute	attr;
caddr_t		op1;
caddr_t		op2;
{   
    switch (attr) {
      case FORM_CURSES_WINDOW:    
	return((caddr_t) form->win);
	
      case FORM_CURRENT_IO_ITEM:
	if (form->cur_io_item) 
	    return((caddr_t) form->cur_io_item->item);
	else
	    return((caddr_t)NULL);
	
      case FORM_FIRST_ROW:
	return((caddr_t) form->first_row);
	
      case FORM_LAST_ROW:
	return((caddr_t) form->last_row);
	
      case FORM_DISPLAY:
	return((caddr_t) form->displayed);
	
      case FORM_WIN_SIZE:
	return((caddr_t) ((form->last_row - form->first_row) + 1));

      case FORM_DISPLAY_FUNC:
	return((caddr_t)form->disp_func);
	break;
	
      default:
	fprintf(stderr, "unknown attr in form_get\n");
	return((caddr_t)NULL);
    }
}

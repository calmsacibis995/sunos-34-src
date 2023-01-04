#ifndef lint
static	char sccsid[] = "@(#)form_item.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "form.h"
#include "object.h"
#include "form_attr.h"
#include "form_match.h"


typedef struct ITEM_INFO {
    int			row, col;	/* location of item's label	 */
    int			hilighted;	/* true if item's label is hilighted */
    int			displayed;	/* true if item should be displayed */
    char		*label;		/* label string			 */
    int			label_len;	/* length of the label string	 */
    caddr_t		client_data;	/* private data of the client 	 */
    Form		form;		/* form that this item is in	 */
    int			(*notify_proc)();/* routine to notify user	 */
    Form_notify_level	notify_level;	/* when to notify the client	 */
    Object		*sub_item;	/* sub_item of this item	 */
    Item		(*sub_item_type)(); /* type of sub-item */
} Item_info;
    

static	void	item_set();
static	caddr_t	item_get();
static	void	item_destroy();
static	void	item_display();
static	int	item_process_input();

extern	int	form_notify_proc();

Cursor_control	cursor_control;



Object	*
item_create(sub_item_create)
Object	*(*sub_item_create)();
{   
    Item_info	*item;
    Object	*obj;
    
    item = (Item_info *) malloc(sizeof(Item_info));

    /* set up item defaults */
    item->row 		= 0;
    item->col 		= 0;
    item->hilighted 	= FALSE;
    item->displayed 	= TRUE;
    item->label 	= (char *) malloc(FORM_DEFAULT_STR_LEN);
    strcpy(item->label, "");
    item->label_len	= strlen(item->label);
    item->client_data	= (caddr_t)NULL;
    item->form		= (Form)NULL;
    item->notify_proc	= form_notify_proc;
    item->notify_level	= FORM_NOTIFY_ON_PICK;
    
    
    obj	= (Object *) malloc(sizeof(Object));

    obj->data	 	= (caddr_t)item;
    obj->set		= item_set;
    obj->get		= item_get;
    obj->destroy	= item_destroy;
    obj->display	= item_display;
    obj->process_input	= item_process_input;
    
    item->sub_item	= (*sub_item_create)(obj); 
    item->sub_item_type = (Item (*)()) sub_item_create;
    
    return(obj);
}



static
void
item_set(obj, avlist)
 		Object	*obj;
register	Form_avlist	avlist;
{   
    register	Item_info	*item;
    register	Form_attribute	attr;
    register	Form_avlist		list_start;
    
    item = (Item_info *) obj->data;
    
    list_start = avlist;
    (*item->sub_item->set)(item->sub_item, list_start);		       
    
    while (attr = (Form_attribute) *avlist++) {
    	switch (attr) {
	  case ITEM_ROW:
	    item->row = (int) *avlist++;
	    break;
	    
	  case ITEM_COL:
	    item->col = (int) *avlist++;
	    break;
	    
	  case ITEM_HILIGHTED:
	    item->hilighted = (int) *avlist++;
	    break;
	   
	  case ITEM_DISPLAYED:
	    item->displayed = (int) *avlist++;
	    break;
	    
	  case ITEM_LABEL:
	    strcpy(item->label, (char *) *avlist++);
	    item->label_len = strlen(item->label);
	    break;
	    
	  case ITEM_CLIENT_DATA:
	    item->client_data = (caddr_t) *avlist++;
	    break;
	    
	  case ITEM_FORM:
	    item->form = (Form) *avlist++;
	    break;
	    
	  case ITEM_NOTIFY_PROC:
	    item->notify_proc = (int (*)()) *avlist++;
	    break;
	    	    
	  case ITEM_NOTIFY_LEVEL:
	    item->notify_level = (Form_notify_level) *avlist++;
	    break;
	    
	    
	  default:
	    avlist = attr_skip(attr, avlist);
	}
	
    }
}



static
caddr_t
item_get(obj, attr, op1, op2)
Object		*obj;
Form_attribute	attr;
caddr_t		op1, op2;
{   
    register	Item_info	*item;
    
    item = (Item_info *) obj->data;
    
    switch (attr) {
      case ITEM_ROW:    
	return((caddr_t) item->row);

      case ITEM_COL:    
	return((caddr_t) item->col);

      case ITEM_HILIGHTED:    
	return((caddr_t) item->hilighted);
	
      case ITEM_DISPLAYED:    
	return((caddr_t) item->displayed);
	
      case ITEM_LABEL:    
	return((caddr_t) item->label);
	
      case ITEM_FORM:
        return((caddr_t) item->form);
	
      case ITEM_CLIENT_DATA:    
	return((caddr_t) item->client_data);
	
      case ITEM_LABEL_LENGTH:
	if (item->label_len == 0)
	    return((caddr_t)0);
	else
	    return((caddr_t) (item->label_len + 3));
	
      case ITEM_TYPE:
	return((caddr_t) item->sub_item_type);

      case ITEM_SUB_ITEM:
	return((caddr_t) item->sub_item);
    }
    
    (*item->sub_item->get)(item->sub_item, attr, op1, op2);
}



static
void
item_display(obj)
Object		*obj;
{       
    register	Item_info	*item;
    		WINDOW		*win;
   		int		row, col;
    register	int		old_row, old_col, i;
    
    item = (Item_info *) obj->data;
    
    if (item_label_calculate_display(obj, &win, &row, &col)) {
	getyx(win, old_row, old_col);
	wmove(win, row, col);
	if ((item->displayed) && (item->label_len > 0)) {
	    if (item->hilighted)
		waddch(win, '('); /* wstandout(win);*/
	    else
		waddch(win, ' ');
	    waddstr(win, item->label);
	    if (item->hilighted)
		waddch(win, ')');
	    else
		waddch(win, ' ');
	    if (item->sub_item_type != FORM_ITEM_BUTTON)
		waddch(win, ' ');
	}
	else if (item->label_len > 0) {
	    waddch(win, ' ');	    
	    for (i = 0; i < item->label_len;  i++)
		waddch(win, ' ');
	    waddch(win, ' ');
	    waddch(win, ' ');
	}
	(*item->sub_item->display)(item->sub_item, item->displayed);
	wmove(win, old_row, old_col);	
    }
}


static
void
item_destroy(obj)
Object		*obj;
{   
    register	Item_info	*item;
    
    item = (Item_info *) obj->data;    
    
    /* undisplay object.... */
    
    (*item->sub_item->destroy)(item->sub_item);
    free(item);
    free(obj);
}



static
int
item_process_input(obj, info)
Object		*obj;
Input_info	*info;
{   
    register	Item_info	*item;
    register	int		ret_val;
		int		item_type;
		caddr_t		sub_item;
    
    item = (Item_info *) obj->data;    
    if (item->notify_level == FORM_NOTIFY_ON_EACH_CHAR) {
	/* it is not clear if "NOTIFY_ON_EACH_CHAR" makes sense */
	/* But!! it is used by the list package! so leave it around */
	return ((*item->notify_proc)(obj, info->str[0]));	
    }
    else {
	cursor_control = CURSOR_CONTROL_NEXT_ITEM;
	if (is_select_action(obj, info)) {
	    ret_val = (*item->notify_proc)(obj, info->str[0]);
	    if (cursor_control == CURSOR_CONTROL_NEXT_ITEM &&
	      info->action == INPUT_SELECT_ACTION) {
		/*
		 * This is fairly kludgy.  If a choice item is 
		 * selected, then the cursor should be positioned
		 * at the next "major" item - not at the next choice.
		 * This is accomplished by moving the cursor to the
		 * last choice and letting the natural cursor movement
		 * move it to the next item.
		 */
		item_type = (int) form_item_get(obj, ITEM_TYPE);
		if (item_type == (int) FORM_ITEM_CHOICE) {
		    sub_item = form_item_get(obj, ITEM_SUB_ITEM);
		    last_choice(sub_item);
		}
		form_goto_item(item->form, FORM_NEXT);
	    }
	    else if (cursor_control == CURSOR_CONTROL_STAY_PUT)
		form_item_set(obj, ITEM_SET_CURSOR, 0);
	    /* else ignore cursor control */
	    return (ret_val);
	}
    }
    return (FALSE);
}

is_select_action(obj, info)
Object	*obj;
Input_info	*info;
{
    Input_info  newinfo;
    Item_info	*item;
    int		item_type;
    int		r;

    item = (Item_info *) obj->data;    
    item_type = (int) form_item_get(obj, ITEM_TYPE);
    newinfo = *info;
    if (item_type == (int) FORM_ITEM_TEXT) {
	if (info->action == INPUT_NEXT_ITEM ||
	  info->action == INPUT_PREV_ITEM ||
	  info->action == INPUT_NEXT_WINDOW ||
	  info->action == INPUT_PREV_WINDOW) {
	    newinfo.action = INPUT_SELECT_ACTION;
	    r = TRUE;
	} else if (info->action == INPUT_SELECT_ACTION) {
	    newinfo.action = INPUT_NO_ACTION;
	    r = FALSE;
	} else {
	    r = FALSE;
	}
    } else if (info->action == INPUT_SELECT_ACTION) {
	r = TRUE;
    } else {
	r = FALSE;
    }
    (*item->sub_item->process_input)(item->sub_item, &newinfo);
    return(r);
}
    


/* 
 * Return TRUE if item is visible and fill in the real row and col.
 */

int
item_label_calculate_display(obj, win, row, col)
Object	*obj;
WINDOW	**win;
int	*row;
int	*col;
{   
    register	Item_info	*item;
    register	int		first_row, last_row;
    
    item = (Item_info *) obj->data;
    
    first_row = (int) form_get(item->form, FORM_FIRST_ROW);
    last_row = (int) form_get(item->form, FORM_LAST_ROW);
    
    if ((item->row >= first_row) && (item->row <= last_row)) {
	*win = (WINDOW *) form_get(item->form, FORM_CURSES_WINDOW);
	*row = item->row - first_row;
	*col = item->col;
	return(TRUE);
    }
    else
	return(FALSE);
}
    
    
    
    

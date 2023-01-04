#ifndef lint
static	char sccsid[] = "@(#)tty_list.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "tty_global.h"
#include "tty_item.h"
#include "tty_list.h"

typedef struct {
    Form	form;		/* form used in displaying this list */
    char	*name;
    Item	items[MAX_ITEMS_IN_LIST];
    int		in_use[MAX_ITEMS_IN_LIST];
    int		num_items;
    int		form_height;
    int		cur_row;
    int		cur_col;
    int		last_col;
    int		first_col;
    int		field_size;
} List_info;


List
list_create(form, field_size, first_col, last_col)
Form	form;
int	field_size;
int	first_col;
int	last_col;
{   
    List_info	*l;
    Item	item;
    
    l = (List_info *) malloc(sizeof(List_info));
    
    l->form_height = (int)form_get(form, FORM_LAST_ROW) - 
	             (int)form_get(form, FORM_FIRST_ROW) + 1;
    
    l->name = "";
    l->num_items = 0;
    l->cur_row = 0;
    l->cur_col = 0; 
    l->first_col = first_col;
    l->last_col = last_col - field_size;
    l->field_size = field_size;
    l->form = form;
    
    return((List)l);
}

list_display(l)
List_info	*l;
{   
    form_set(l->form, FORM_DISPLAY, TRUE, 0);
}

list_undisplay(l)
List_info	*l;
{   
    touchwin((WINDOW *)form_get(l->form, FORM_CURSES_WINDOW));
    wclear((WINDOW *)form_get(l->form, FORM_CURSES_WINDOW));
}




list_add(l, text)
register	List_info	*l;
 		char		*text;
{   
    		Item	item;
    register	int	i;
    
    for (i = 0; i < l->num_items; i++) {
	if (!(l->in_use[i])) {
	    l->in_use[i] = TRUE;
	    form_item_set(l->items[i], ITEM_VALUE, text, 0);
	    form_item_display(l->items[i]);
	    form_set(l->form, FORM_REFRESH, 0);
	    return;
	}
    }
    
    if ((l->num_items + 1) == MAX_ITEMS_IN_LIST) {
	fprintf(stderr, "Too many items in list.\n");
	return;
    }
    if (l->cur_col > l->last_col) {
	l->cur_row++;
	l->cur_col = 0;
	item = form_item_create(l->form, 
				FORM_ITEM_TEXT, 
				FORM_INPUT_OUTPUT_ITEM, 
				ITEM_ROW, 		l->cur_row, 
				ITEM_COL, 		l->cur_col, 
				ITEM_TEXT_FIELD_SIZE,	(l->field_size -1), 
				ITEM_VALUE, 		text, 
				ITEM_NOTIFY_LEVEL, 	FORM_NOTIFY_ON_EACH_CHAR, 
				0);
	form_set(l->form, FORM_CURRENT_IO_ITEM, item, 0);
	form_set(l->form, FORM_DISPLAY_HACK, 0);
	if ((int)form_get(l->form, FORM_DISPLAY) == TRUE)
	    form_set(l->form, FORM_DISPLAY, TRUE, 0);
    }
    else {
	if (l->num_items == 0) {
	    item = form_item_create(l->form, 
				    FORM_ITEM_TEXT, 
				    FORM_INPUT_OUTPUT_ITEM, 
				    ITEM_ROW, 		l->cur_row, 
				    ITEM_COL, 		l->cur_col, 
				    ITEM_TEXT_FIELD_SIZE, (l->field_size -1), 
				    ITEM_VALUE, 	text, 
				    ITEM_NOTIFY_LEVEL, 	FORM_NOTIFY_ON_EACH_CHAR, 
				    0);
	    form_set(l->form, FORM_CURRENT_IO_ITEM, item, 0);	
	}
	else {
	    item = form_item_create(l->form, 
				    FORM_ITEM_TEXT, 
				    FORM_OUTPUT_ONLY_ITEM, 
				    ITEM_ROW, 		l->cur_row, 
				    ITEM_COL, 		l->cur_col, 
				    ITEM_TEXT_FIELD_SIZE, (l->field_size -1), 
				    ITEM_VALUE, 	text, 
				    0);
	}
    }
    l->cur_col += l->field_size;
    l->items[l->num_items] = item;
    l->in_use[l->num_items] = TRUE;
    l->num_items++;
    form_item_display(item);
    form_set(l->form, FORM_REFRESH, 0);
    
}



list_remove(l, text)
register	List_info	*l;
 		char		*text;
{   
    Item	item;
    register	int	i;
    
    for (i = 0; i < l->num_items; i++) {
	if ((l->in_use[i]) && 
	    (strcmp(form_item_get(l->items[i], ITEM_VALUE), text) == 0)) {
	    form_item_set(l->items[i], ITEM_VALUE, "", 0);
	    form_item_display(l->items[i]);
	    form_set(l->form, FORM_REFRESH, 0);
	    l->in_use[i] = FALSE;
	    return;
	}
    }
}

		
		
    

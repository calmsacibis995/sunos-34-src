#ifndef lint
static	char sccsid[] = "@(#)form_toggle.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "form.h"
#include "object.h"
#include "form_attr.h"
#include "form_match.h"
#include "user_info.h"
#include <ctype.h>

#define MAX_NUM_TOGGLES		65

#define	TOGGLE_YES(t, n)  t->states |= (1 << n)
#define TOGGLE_NO(t, n)   t->states &= ~(1 << n)
#define TOGGLE_IS_YES(t, n) ((t->states >> n) & 0x01)


typedef struct {
    Object	*parent;
    char	*toggles[MAX_NUM_TOGGLES]; /* toggle strings	 */
    int		toggle_len[MAX_NUM_TOGGLES]; 
    int		states;
    int		max_len;
    int		looking_at;
    int		num_toggles;
    int		cycle_display;
    int		column;
} Toggle_info;

static	void	toggle_set();
static	caddr_t	toggle_get();
static	void	toggle_destroy();
static	void	toggle_display();
static	int	toggle_process_input();



Item
toggle_create(parent)
Object	*parent;
{   
    Toggle_info		*toggle;
    Object		*obj;
    int			i;
    
    toggle = (Toggle_info *) malloc(sizeof(Toggle_info));
    
    toggle->parent = parent;
    toggle->num_toggles = 0;
    toggle->looking_at  = 0;
    toggle->max_len	= 0;
    toggle->cycle_display  = FALSE;
    toggle->states	= 0;
    toggle->column	= -1;
    
    for (i = 0; i < MAX_NUM_TOGGLES; i++)
	toggle->toggles[i] = (char *)NULL;
	 
    obj = (Object *) malloc(sizeof(Object));
    
    obj->data 		= (caddr_t) toggle;
    obj->set 		= toggle_set;
    obj->get 		= toggle_get;
    obj->destroy 	= toggle_destroy;
    obj->display 	= toggle_display;
    obj->process_input 	= toggle_process_input;
    
    return((Item)obj);
}



static
void
toggle_set(obj, avlist)
Object	*obj;
register	Form_avlist	avlist;
{   
    register	Toggle_info	*toggle;
    register	Form_attribute	attr;
    		WINDOW		*win;
   		int		row, col;
    		int		label_len;
    register	int		i, toggle_len;
    
    toggle = (Toggle_info *) obj->data;
    
    while (attr = (Form_attribute) *avlist++) {
	switch (attr) {
	  case ITEM_SET_CURSOR:
	    if (item_label_calculate_display(toggle->parent, 
					     &win, &row, &col)) {
		/* 4 chars before each toggle string and 2 between */
		if (toggle->cycle_display) {
		    toggle_len = 3;
		}
		else {
		    toggle_len = 0;
		    for (i = 0; i < toggle->looking_at; i++)
			toggle_len += toggle->toggle_len[i];
		    i = toggle->looking_at;
		    toggle_len += (3 * (i + 1)) + (i * 3);
		}
		label_len = (int)form_item_get(toggle->parent,
					       ITEM_LABEL_LENGTH);
		if (toggle->column == -1)
		    wmove(win, row, (col + toggle_len + label_len));
		else
		    wmove(win, row, (toggle->column + toggle_len));
		wrefresh(win);
	    }
	    break;
	    
	  case ITEM_TOGGLE_STRING:
	    i = toggle->num_toggles++;
	    if (i >= MAX_NUM_TOGGLES) {
		fprintf(stderr, "form_toggle.c: too many toggle strings.\n");
		i =  MAX_NUM_TOGGLES - 1;
	    }
	    if (toggle->toggles[i] != (char *)NULL)
                free(toggle->toggles[i]);
       
	    toggle->toggles[i] = malloc(strlen(*avlist) + 2);
	    strcpy(toggle->toggles[i], (char *) *avlist++);
	    toggle->toggle_len[i] = strlen(toggle->toggles[i]);
	    if (toggle->toggle_len[i] >= toggle->max_len)
		toggle->max_len = toggle->toggle_len[i];
	    break;
	    
	  case ITEM_TOGGLE_CYCLE_DISPLAY:
	    toggle->cycle_display = (int) *avlist++;
	    break;
	    
	  case ITEM_VALUE:
	    toggle->states = (int) *avlist++;
	    break;
	    
	  case ITEM_VALUE_COL:
	    toggle->column = (int) *avlist++;
	    break;
	    
	  case ITEM_TOGGLE_RESET:
	    toggle->num_toggles = 0;
	    
	  default:
	    avlist = attr_skip(attr, avlist);
	    
	}
    }
}



    
static
caddr_t
toggle_get(obj, attr )
Object		*obj;
Form_attribute	attr;
{   
    register	Toggle_info	*toggle;
    
    toggle = (Toggle_info *) obj->data;
    
    switch (attr) {
      case ITEM_VALUE:
	return( (caddr_t) toggle->states);
	
     /* default:
	 fprintf(stderr, "Unknown att in toggle_get\n"); */
	    
    }
}


   

static
void
toggle_destroy(obj)
Object	*obj;
{   
    free(obj->data);
    free(obj);
}
    


static
void
toggle_display(obj, display)
Object	*obj;
int	display;
{   
    register	Toggle_info	*toggle;
    register	int		i, tmp_row, tmp_col, len;
    register	WINDOW		*win;
    
    toggle = (Toggle_info *) obj->data;
    
    win = (WINDOW *) form_get((Form)form_item_get(toggle->parent, ITEM_FORM), 
		   FORM_CURSES_WINDOW);
    getyx(win, tmp_row, tmp_col);
    if (toggle->column != -1)
	wmove(win, tmp_row, toggle->column);
    if (display) {
	if (toggle->cycle_display) {
	    draw_toggle(toggle, toggle->looking_at, win);   
	}
	else {
	    for (i = 0; i < toggle->num_toggles; i++) {
		draw_toggle(toggle, i, win);
	    }
	}
    }
    else {
	if (toggle->cycle_display) {
	    len = 4 + toggle->max_len;
	}
	else {
	    len = 0;
	    for (i = 0; i < toggle->num_toggles; i++) 
		len += toggle->toggle_len[i];
	    i = toggle->num_toggles;
	    len += (4 * i) + ((i - 1) * 2);
	}
	for (i = 0; i < len; i++) 
	    waddch(win, ' ');
    }
    wmove(win, tmp_row, tmp_col);
}



draw_toggle(toggle, i, win)
register	Toggle_info	*toggle;
register	int		i;
register	WINDOW		*win;
{   
    waddch(win, '<');
    if (TOGGLE_IS_YES(toggle, i))
	waddch(win, 'X'); 
    else
	waddch(win, ' '); 
    waddstr(win, "> ");
    waddstr(win, toggle->toggles[i]);
    if ((i < (toggle->num_toggles - 1)) &&
	(!toggle->cycle_display))
	waddstr(win, ", ");       
}




static
int
toggle_process_input(obj, info)
Object		*obj;
Input_info	*info;
{   
    register	Toggle_info	*toggle;
    register	int		num_to_erase;
    register	int		i, cur_row, cur_col;
    register	WINDOW		*win;
    		char		m;
    extern	Cursor_control	cursor_control;
    
    toggle = (Toggle_info *) obj->data;
    
    if (toggle->num_toggles == 0)
	return;
    win = (WINDOW *) form_get((Form)form_item_get(toggle->parent, ITEM_FORM), 
		   FORM_CURSES_WINDOW);
    if (info->action == INPUT_SELECT_ACTION) {	
	if (TOGGLE_IS_YES(toggle,toggle->looking_at))
	    TOGGLE_NO(toggle,toggle->looking_at);
	else
	    TOGGLE_YES(toggle,toggle->looking_at);
	form_item_display(toggle->parent);
	form_set((Form)form_item_get(toggle->parent, ITEM_FORM), 
		 FORM_REFRESH, 0);
    }    
    else {
	if ((info->action == INPUT_NO_ACTION) && (info->str[0] == ' ')) {
	    toggle->looking_at++;
	    if (toggle->looking_at == toggle->num_toggles)
		toggle->looking_at = 0;
	}
	else if (info->action == INPUT_CHAR_DEL) {
	    toggle->looking_at--;
	    if (toggle->looking_at == -1 )
		toggle->looking_at = toggle->num_toggles - 1;
	}   
	if (toggle->cycle_display)
	    form_item_display(toggle->parent);
	form_item_set(toggle->parent, ITEM_SET_CURSOR, 0);
    }
}
 
/*
 * For fully displayed toggle lists, make the next member of the list
 * the center of attention.  Return false if there are no more items
 * in the list.
 */
next_toggle(obj)
Object	*obj;
{
    Toggle_info	*toggle;
	
    toggle = (Toggle_info *) obj->data;
    if (toggle->cycle_display == 0) {
        toggle->looking_at++;
        if (toggle->looking_at == toggle->num_toggles) {
	    toggle->looking_at = 0;
	    return(FALSE);
        } else {
	    form_item_set(toggle->parent, ITEM_SET_CURSOR, 0);
            return(TRUE);
	}
    } else {
	return(FALSE);
    }
}
 
/*
 * For fully displayed toggle lists, make the previous member of the list
 * the center of attention.  Return false if there are no previous items
 * in the list.
 */
prev_toggle(obj)
Object	*obj;
{
    Toggle_info	*toggle;
	
    toggle = (Toggle_info *) obj->data;
    if (toggle->cycle_display == 0) {
        toggle->looking_at--;
        if (toggle->looking_at == -1) {
	    toggle->looking_at = 0;
	    return(FALSE);
        } else {
	    form_item_set(toggle->parent, ITEM_SET_CURSOR, 0);
            return(TRUE);
	}
    } else {
	return(FALSE);
    }
}

/*
 * For fully display toggle lists, make the last toggle item
 * the current one.
 */
last_toggle(obj)
Object	*obj;
{
    Toggle_info	*toggle;
	
    toggle = (Toggle_info *) obj->data;
    if (toggle->cycle_display == 0) {
        toggle->looking_at = toggle->num_toggles - 1;
	form_item_set(toggle->parent, ITEM_SET_CURSOR, 0);
    }
}

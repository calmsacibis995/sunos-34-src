#ifndef lint
static	char sccsid[] = "@(#)form_choice.c 1.1 86/09/25 Copyr 1985 Sun Micro";
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

#define MAX_NUM_CHOICES		65

typedef struct {
    Object	*parent;
    char	*choices[MAX_NUM_CHOICES]; /* choice strings	 */
    int		choice_len[MAX_NUM_CHOICES]; 
    int		max_len;
    int		cur_choice;
    int		looking_at;
    int		num_choices;
    int		cycle_display;
    int		column;
} Choice_info;

static	void	choice_set();
static	caddr_t	choice_get();
static	void	choice_destroy();
static	void	choice_display();
static	int	choice_process_input();
static	int	draw_choice();



Item
choice_create(parent)
Object	*parent;
{   
    Choice_info		*choice;
    Object		*obj;
    register int	i;
    
    choice = (Choice_info *) malloc(sizeof(Choice_info));
    
    choice->parent = parent;
    choice->cur_choice  = 0;
    choice->num_choices = 0;
    choice->looking_at  = 0;
    choice->max_len	= 0;
    choice->cycle_display  = FALSE;
    choice->column	= -1;
    
    for (i = 0; i < MAX_NUM_CHOICES; i++)
	choice->choices[i] = (char *)NULL;
    
    obj = (Object *) malloc(sizeof(Object));
    
    obj->data 		= (caddr_t) choice;
    obj->set 		= choice_set;
    obj->get 		= choice_get;
    obj->destroy 	= choice_destroy;
    obj->display 	= choice_display;
    obj->process_input 	= choice_process_input;
    
    return((Item)obj);
}



static
void
choice_set(obj, avlist)
Object	*obj;
register	Form_avlist	avlist;
{   
    register	Choice_info	*choice;
    register	Form_attribute	attr;
    		WINDOW		*win;
   		int		row, col;
    		int		label_len;
    register	int		i, choice_len;
    
    choice = (Choice_info *) obj->data;
    
    while (attr = (Form_attribute) *avlist++) {
	switch (attr) {
	  case ITEM_SET_CURSOR:
	    if (item_label_calculate_display(choice->parent, 
					     &win, &row, &col)) {
		/* 4 chars before each choice string and 2 between */
		if (choice->cycle_display) {
		    choice_len = 3;
		}
		else {
		    choice_len = 0;
		    for (i = 0; i < choice->looking_at; i++)
			choice_len += choice->choice_len[i];
		    i = choice->looking_at;
		    choice_len += (3 * (i + 1)) + (i * 3);
		}
		label_len =  (int) form_item_get(choice->parent, 
						    ITEM_LABEL_LENGTH);
		if (choice->column == -1)
		    wmove(win, row, (col + choice_len + label_len));
		else
		    wmove(win, row, (choice->column + choice_len));
		wrefresh(win);
	    }
	    break;
	    
	  case ITEM_CHOICE_STRING:
	    i = choice->num_choices++;
	    if (i >= MAX_NUM_CHOICES) {
		fprintf(stderr, "form_choice.c: too many choice strings.\n");
		i =  MAX_NUM_CHOICES - 1;
	    }
	    if (choice->choices[i] != (char *)NULL)
		free(choice->choices[i]);
	    
	    choice->choices[i] = malloc(strlen(*avlist) + 2);
	    strcpy(choice->choices[i], (char *) *avlist++);
	    choice->choice_len[i] = strlen(choice->choices[i]);
	    if (choice->choice_len[i] >= choice->max_len)
		choice->max_len = choice->choice_len[i];
	    break;
	    
	  case ITEM_CHOICE_CYCLE_DISPLAY:
	    choice->cycle_display = (int) *avlist++;
	    break;
	    
	  case ITEM_CHOICE_RESET:
	    choice->num_choices = 0;
	    break;
	    
	  case ITEM_VALUE:
	    choice->cur_choice = (int) *avlist++;
	    if (choice->cycle_display)
	        choice->looking_at = choice->cur_choice;
	    break;
	    
	  case ITEM_VALUE_COL:
	    choice->column = (int) *avlist++;
	    break;

	  default:
	    avlist = attr_skip(attr, avlist);
	    
	}
    }
}



    
static
caddr_t
choice_get(obj, attr )
Object		*obj;
Form_attribute	attr;
{   
    register	Choice_info	*choice;
    
    choice = (Choice_info *) obj->data;
    
    switch (attr) {
      case ITEM_VALUE:
	return( (caddr_t) choice->cur_choice);
	
      case ITEM_VALUE_COL:
	return((caddr_t) choice->column);

     /* default:
	 fprintf(stderr, "Unknown att in choice_get\n"); */
	    
    }
}


   

static
void
choice_destroy(obj)
Object	*obj;
{   
    free(obj->data);
    free(obj);
}
    


static
void
choice_display(obj, display)
Object	*obj;
int	display;
{   
    register	Choice_info	*choice;
    register	int		i, tmp_row, tmp_col, len;
    register	WINDOW		*win;
    
    choice = (Choice_info *) obj->data;
    
    win = (WINDOW *) form_get((Form)form_item_get(choice->parent, ITEM_FORM), 
		   FORM_CURSES_WINDOW);
    getyx(win, tmp_row, tmp_col);
    if (choice->column != -1)
	wmove(win, tmp_row, choice->column);
    
    if (display) {
	if (choice->cycle_display) {
	    draw_choice(choice, choice->looking_at, win);   
	}
	else {
	    for (i = 0; i < choice->num_choices; i++) {
		draw_choice(choice, i, win);
	    }
	}
    }
    else {
	if (choice->cycle_display) {
	    len = 4 + choice->max_len;
	}
	else {
	    len = 0;
	    for (i = 0; i < choice->num_choices; i++) 
		len += choice->choice_len[i];
	    i = choice->num_choices;
	    len += (4 * i) + ((i - 1) * 2);
	}
	for (i = 0; i < len; i++) 
	    waddch(win, ' ');
    }
    wmove(win, tmp_row, tmp_col);
}


static
int
draw_choice(choice, i, win)
register	Choice_info	*choice;
register	int		i;
register	WINDOW		*win;
{   
    register	int	j;
    
    if (choice->choices[i] == (char *)NULL)
	return;
    
    waddch(win, '[');
    if (i == choice->cur_choice)
	waddch(win, 'X'); 
    else
	waddch(win, ' '); 
    waddstr(win, "] ");
    waddstr(win, choice->choices[i]);
    if (choice->cycle_display) {
	for (j = choice->choice_len[i] + 1 ; j <= choice->max_len; j++)
	    waddch(win, ' ');	
    }
    else if (i < (choice->num_choices - 1))
	waddstr(win, ", ");
}




static
int
choice_process_input(obj, info)
Object		*obj;
Input_info	*info;
{   
    register	Choice_info	*choice;
    register	int		num_to_erase;
    register	int		i, cur_row, cur_col;
    register	WINDOW		*win;
    		char		m;
    
    choice = (Choice_info *) obj->data;
    
    if (choice->num_choices == 0)
	return;
    win = (WINDOW *) form_get((Form)form_item_get(choice->parent, ITEM_FORM), 
		   FORM_CURSES_WINDOW);
    if (info->action == INPUT_SELECT_ACTION) {
	choice->cur_choice = choice->looking_at;
	form_item_display(choice->parent);
	form_set((Form)form_item_get(choice->parent, ITEM_FORM), 
		 FORM_REFRESH, 0);
    }
    else {
	if ((info->action == INPUT_NO_ACTION) && (info->str[0] == ' ')) {
	    choice->looking_at++;
	    if (choice->looking_at == choice->num_choices)
		choice->looking_at = 0;
	}
	else if (info->action == INPUT_CHAR_DEL) {
	    choice->looking_at--;
	    if (choice->looking_at == -1)
		choice->looking_at = choice->num_choices - 1;
	}
	if (choice->cycle_display)
	    form_item_display(choice->parent);
	form_item_set(choice->parent, ITEM_SET_CURSOR, 0);
    }
}
 
/*
 * For fully displayed choice items, make the next member of 
 * the choice item the center of attention.
 * Return false if there are no more items.
 */
next_choice(obj)
Object	*obj;
{
    Choice_info	*choice;
	
    choice = (Choice_info *) obj->data;
    if (choice->cycle_display == 0) {
        choice->looking_at++;
        if (choice->looking_at == choice->num_choices) {
	    choice->looking_at = 0;
	    return(FALSE);
        } else {
	    form_item_set(choice->parent, ITEM_SET_CURSOR, 0);
            return(TRUE);
	}
    } else {
	return(FALSE);
    }
}
 
/*
 * For fully displayed choice items, make the previous member of 
 * the choice item the center of attention.
 * Return false if there are no previous items.
 */
prev_choice(obj)
Object	*obj;
{
    Choice_info	*choice;
	
    choice = (Choice_info *) obj->data;
    if (choice->cycle_display == 0) {
        choice->looking_at--;
        if (choice->looking_at == -1) {
	    choice->looking_at = 0;
	    return(FALSE);
        } else {
	    form_item_set(choice->parent, ITEM_SET_CURSOR, 0);
            return(TRUE);
	}
    } else {
	return(FALSE);
    }
}

/*
 * For fully displayed choice items, make the last choice 
 * the current one.
 */
last_choice(obj)
Object	*obj;
{
    Choice_info	*choice;
	
    choice = (Choice_info *) obj->data;
    if (choice->cycle_display == 0) {
        choice->looking_at = choice->num_choices - 1;
	form_item_set(choice->parent, ITEM_SET_CURSOR, 0);
    }
}

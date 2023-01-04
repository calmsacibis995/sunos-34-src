#ifndef lint
static	char sccsid[] = "@(#)form_input.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "tty_global.h"
#include "user_info.h"
#include "form_match.h"

#define FORM_NUM_NEXT_ITEM_CHARS	3
#define FORM_NUM_PREVIOUS_ITEM_CHARS	2


extern	Cursor_control	cursor_control;

form_process_input(forms, num_forms, cur_form)
Form	*forms[];
int	num_forms;
int	cur_form;
{   
    		char		ch;
    register 	int		done;
    register	Item		cur_item;
    		int		first_form;
    		char		buff[100];
    register	int		n_chars;
    		Input_action	action;
    		Match_type	match_result;
		Input_info	in_info;
		
    done = FALSE;
    first_form = cur_form;
    do {
	if ((cur_item = (Item)form_get(forms[cur_form],FORM_CURRENT_IO_ITEM))
	    != NULL) {
	    form_item_set(cur_item, ITEM_SET_CURSOR, 0);
	    done = TRUE;
	}
	else {
	    if (++cur_form == num_forms)
		cur_form = 0;
	}
    } while ((cur_form != first_form) && (!done)); 
    
    if ((cur_form == first_form) && (!done))
	return;
    buff[0] = '\0';
    n_chars = 0;
    while (read(fileno(stdin), &ch, 1) == 1) {
	if (ch == 0) 
	    continue; 	/* don't bother with the null-character */
	buff[n_chars++] = (char) ch;
	buff[n_chars] = '\0';
	match_result = form_input_match(buff, &action);
	in_info.action = action;
	in_info.str = buff;
	if (match_result == MATCH) {
	    switch (action) {
	      case INPUT_NEXT_WINDOW:
	      case INPUT_PREV_WINDOW:
	        if (form_item_process_input(cur_item, &in_info)) {
		    return;
		}
		if (cursor_control == CURSOR_CONTROL_NEXT_ITEM) {
		    done = FALSE;
		    while (!done) {
		        if (action == INPUT_NEXT_WINDOW) { /* next */
			    if (++cur_form == num_forms)
			        cur_form = 0;
		        }
		        else if (action == INPUT_PREV_WINDOW) { /* prev */
			    if (--cur_form == -1)
			        cur_form = num_forms - 1;
		        }
		        if ((cur_item = (Item) form_get(forms[cur_form],
			  FORM_CURRENT_IO_ITEM)) != NULL) {
			    form_item_set(cur_item, ITEM_SET_CURSOR, 0);
			    done = TRUE;
		        }
		    }
		}
		break;
		
	      case INPUT_NEXT_ITEM:
	        if (form_item_process_input(cur_item, &in_info)) {
		    return;
		}
		if (cursor_control == CURSOR_CONTROL_NEXT_ITEM) {
		    form_goto_item(forms[cur_form], FORM_NEXT);
		}
		cur_item = (Item) form_get(forms[cur_form],
		  FORM_CURRENT_IO_ITEM);
		break;
		
	      case INPUT_PREV_ITEM:
	        if (form_item_process_input(cur_item, &in_info)) {
		    return;
		}
		if (cursor_control == CURSOR_CONTROL_NEXT_ITEM) {
		    form_goto_item(forms[cur_form], FORM_PREV);
		}
		cur_item = (Item) form_get(forms[cur_form],
		  FORM_CURRENT_IO_ITEM);
		break;

	      case INPUT_REFRESH:
		touchwin(curscr);
		wrefresh(curscr);
		break;

	      default:
		if (form_item_process_input(cur_item, &in_info)) {
		    return ;    
		}
		cur_item = (Item) form_get(forms[cur_form],FORM_CURRENT_IO_ITEM);
		break;
	    }
	    buff[0] = '\0';
	    n_chars = 0;	    
	}
	else if (match_result == NO_MATCH) {
	    in_info.action = INPUT_NO_ACTION;
	    in_info.str = buff;
	    if (form_item_process_input(cur_item, &in_info)) {
		return ;    
	    }
	    cur_item = (Item) form_get(forms[cur_form],FORM_CURRENT_IO_ITEM);
	    buff[0] = '\0';
	    n_chars = 0;	    
	}   
    }	/* end while */
}


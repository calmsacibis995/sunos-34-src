#ifndef lint
static	char sccsid[] = "@(#)form_text.c 1.1 86/09/25 Copyr 1985 Sun Micro";
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


extern	User_info	user_info;

typedef struct {
    Object	*parent;
    char	*text;		/* text string			 */
    int		text_len;	/* number of chars in text item  */
    int		field_size;	/* max number of chars in item	 */    
    int		start_ch;
    int		column;
} Text_info;

static	void	text_set();
static	caddr_t	text_get();
static	void	text_destroy();
static	void	text_display();
static	int	text_process_input();



Item
text_create(parent)
Object	*parent;
{   
    Text_info		*text;
    Object		*obj;
    
    text = (Text_info *) malloc(sizeof(Text_info));
    
    text->parent	= parent;
    text->text		= malloc(FORM_DEFAULT_STR_LEN);
    strcpy(text->text, "");
    text->text_len	= 0;
    text->field_size	= 8;
    text->start_ch	= 0;
    text->column	= -1;
    
    obj = (Object *) malloc(sizeof(Object));
    
    obj->data 		= (caddr_t) text;
    obj->set 		= text_set;
    obj->get 		= text_get;
    obj->destroy 	= text_destroy;
    obj->display 	= text_display;
    obj->process_input 	= text_process_input;
    
    return((Item)obj);
}



static
void
text_set(obj, avlist)
Object	*obj;
register	Form_avlist	avlist;
{   
    register	Text_info	*text;
    register	Form_attribute	attr;
    		WINDOW		*win;
   		int		row, col;
		char		*str;
    register	int		label_len, t_len;
    
    text = (Text_info *) obj->data;
    
    while (attr = (Form_attribute) *avlist++) {
	switch (attr) {
	  case ITEM_SET_CURSOR:
	    if (item_label_calculate_display(text->parent, 
					     &win, &row, &col)) {
		label_len = (int)form_item_get(text->parent, ITEM_LABEL_LENGTH);
		if (text->text_len > text->field_size)
		    t_len = text->field_size;
		else
		    t_len = text->text_len;
		if (text->column == -1)
		    wmove(win, row, (col + t_len + label_len));
		else
		    wmove(win, row, (text->column + t_len));
		wrefresh(win);
	    }
	    break;
	    
	  case ITEM_VALUE:
	    str = (char *) *avlist++;
	    if (str == NULL)
		str = "";
	    strcpy(text->text, str);
	    text->text_len = strlen(text->text);
	    break;
	    
	  case ITEM_TEXT_FIELD_SIZE:
	    text->field_size = (int) *avlist++;
	    break;
	    
	  case ITEM_VALUE_COL:
	    text->column = (int) *avlist++;
	    break;
	  
	  
	  default:
	    avlist = attr_skip(attr, avlist);
	    
	}
    }
}



    
static
caddr_t
text_get(obj, attr )
Object		*obj;
Form_attribute	attr;
{   
    register	Text_info	*text;
    
    text = (Text_info *) obj->data;
    
    switch (attr) {
      case ITEM_VALUE:
	return( (caddr_t) text->text);
	
     /* default:
	 fprintf(stderr, "Unknown att in text_get\n"); */
	    
    }
}


   

static
void
text_destroy(obj)
Object	*obj;
{   
    free(obj->data);
    free(obj);
}
    


static
void
text_display(obj, display)
Object	*obj;
int	display;
{   
    register	Text_info	*text;
    register	int		i, end;
    register	WINDOW		*win;
    register	int		tmp_row, tmp_col;
    
    text = (Text_info *) obj->data;
    
    win = (WINDOW *) form_get((Form)form_item_get(text->parent, ITEM_FORM), 
		   FORM_CURSES_WINDOW);
    getyx(win, tmp_row, tmp_col);
    if (text->column != -1)
	wmove(win, tmp_row, text->column);
    
    if (display) {
	if (text->text_len > text->field_size)
	    text->start_ch = text->text_len - text->field_size;
	else
	    text->start_ch = 0;
	end = text->field_size + text->start_ch;
	for (i = text->start_ch; (text->text[i] != '\0') && (i < end); i++)
	    waddch(win, text->text[i]);
	for (; i < end; i++)
	    waddch(win, ' ');
    }
    else {
	for (i = 0; i < text->field_size; i++)
	    waddch(win, ' ');
    }

}



static
int
text_process_input(obj, info)
Object		*obj;
Input_info	*info;
{   
    register	Text_info	*text;
    register	int		num_to_erase;
    register	int		i, cur_row, cur_col;
    register	WINDOW		*win;
    		char		ch;
    
    text = (Text_info *) obj->data;

    if (info->action == INPUT_SELECT_ACTION) 
	return;
    
    win = (WINDOW *) form_get((Form)form_item_get(text->parent, ITEM_FORM), 
		   FORM_CURSES_WINDOW);
    ch = info->str[0];
    getyx(win, cur_row, cur_col);
    if (info->action == INPUT_NO_ACTION) {
	if (isascii(ch) && isprint(ch)) {
	    if ((text->text_len + 1) <  FORM_DEFAULT_STR_LEN) {
		text->text[text->text_len++] = (char)ch;
		text->text[text->text_len] =  '\0';	
		if (text->text_len > text->field_size) {
		    wmove(win, cur_row, (cur_col - text->field_size));
		    text_display(obj, TRUE);
		}
		else {
		    waddch(win, ch);
		}
	    }
	    else {
		tty_ding_user(); /* no more room */
	    }
	}
    }
    else if (text->text_len > 0) {
	switch (info->action) {

	  case INPUT_CHAR_DEL:
	    num_to_erase = 1;
	    break;
	    
	  case INPUT_WORD_DEL:
	    i = text->text_len - 1;
	    while (text->text[i] == ' ') {
		i--;	/* find end of word  */
	    }
	    while ((i >=  0) && (text->text[i] != ' ')) {
		i--;	/* find beginning of word */
	    }
	    num_to_erase = text->text_len - (i + 1);
	    break;
	    
	    
	  case INPUT_LINE_DEL:
	    num_to_erase = text->text_len;
	    break;
	  
	  default:
	    return;	/* ignore input */
	}
	
	text->text_len -= num_to_erase;
	text->text[text->text_len] = '\0';
	    
	if (text->start_ch > 0) {
	    wmove(win, cur_row, (cur_col - text->field_size));
	    text_display(obj, TRUE);
	    if (text->start_ch == 0) {
		wmove(win, cur_row, ((cur_col - text->field_size) + text->text_len));
	    }
	}
	else {
	    cur_col -= num_to_erase;
	    wmove(win, cur_row, cur_col);
	    for (i = 0; i < num_to_erase; i++)
		waddch(win, ' ');
	    wmove(win, cur_row, cur_col);
	}
    }
    wrefresh(win);
}




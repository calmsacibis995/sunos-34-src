#ifndef lint
static	char sccsid[] = "@(#)tty_error.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "tty_global.h"
#include "tty_item.h"

static	Form	error_form;
static	int	item_row;
Item	label;

tty_error_init(form)
Form	form;
{   Item	item;
    
    error_form = form;
    item_row = 1;
    
    item = BUTTON_ITEM(error_form, 0, 0, "Message Log:");
    form_set(error_form, FORM_CURRENT_IO_ITEM, item, 0);
    
}


tty_error_msg(msg)
char	*msg;
{   
    tty_msg(msg);
    tty_ding_user();
}

tty_msg_user(msg)
char	*msg;
{   
    tty_msg(msg);
}


tty_msg(msg)
char	*msg;
{   
    Item	text;
    char	buff[30];
    char	buff2[80];
    int		row, col;

    getyx(curscr, row, col);

    sprintf(buff, "%3d:", item_row);
    label = BUTTON_ITEM(error_form, item_row, 0, buff);
    if (strlen(msg) > 75)
	strncpy(buff2, msg, 75);
    else
	strcpy(buff2, msg);
    
    text = form_item_create(error_form, 
			    FORM_ITEM_TEXT, 
			    FORM_OUTPUT_ONLY_ITEM, 
			    ITEM_ROW, 		item_row, 
			    ITEM_COL, 		strlen(buff) + 2, 
			    ITEM_TEXT_FIELD_SIZE, 80, 
			    ITEM_VALUE, 	buff2, 
			    0);
    
    item_row++;
    tty_error_display_last_msg();

    wmove(stdscr, row, col);
    wrefresh(stdscr);

}

message_print(msg)
char	*msg;
{   
    tty_error_msg(msg);
}



tty_ding_user()
{   
    char	m;
    
    m = 7;
    write(fileno(stdout), &m, 1);
}

tty_error_display_last_msg()
{   
    if (label == NULL)
	return;
    
    form_set(error_form, FORM_CURRENT_IO_ITEM, label, 0); 
    form_set(error_form, FORM_DISPLAY_HACK, 0);
    form_set(error_form, FORM_DISPLAY, TRUE, 0);
}

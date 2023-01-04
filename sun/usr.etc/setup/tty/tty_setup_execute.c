#ifndef lint
static	char sccsid[] = "@(#)tty_setup_execute.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
* Copyright (c) 1985 by Sun Microsystems, Inc.
*/

#include "tty_global.h"
#include "tty_item.h"


extern	Workstation	ws;
extern	Form	nav_form;
extern	Form	error_form;
extern	WINDOW	*line1_win;
extern	WINDOW	*setup_error_win;
extern	WINDOW	*setup_error_big_win;
extern	WINDOW	*setup_line_win;
extern	WINDOW	*setup_win;

extern	Form	con_form;

/* 
#define DEBUG  
/**/

tty_setup_cmd(cmd)
int	cmd;
{   
    
    form_set(error_form, 
	     FORM_CURSES_WINDOW, setup_error_win, 
	     FORM_FIRST_ROW, 	0, 
	     FORM_WIN_SIZE, 	SETUP_ERROR_WIN_SIZE, 
	     0);
    tty_error_display_last_msg();
    touchwin(setup_line_win);
    wrefresh(setup_line_win);
    form_display(con_form);

#ifdef DEBUG
    if (tty_confirm_proc("Show continue_proc?")) {
	sleep(2);
	tty_continue_proc("This is the continue_proc.");
	sleep(2);
    }
#else
    if (cmd == TTY_CMD_SETUP_EXECUTE) {
	setup_execute(ws);
    }
    else if (cmd == TTY_CMD_SETUP_REBOOT) {
	if (tty_confirm_proc("Reboot. Are you sure?"))
	    setup_reboot(ws);
    }
#endif
	
    form_set(error_form, 
	     FORM_CURSES_WINDOW, setup_error_big_win, 
	     FORM_FIRST_ROW, 	0, 
	     FORM_WIN_SIZE, 	SETUP_ERROR_BIG_WIN_SIZE, 
	     0);
    
    tty_error_display_last_msg();
    
    form_display(nav_form);
    touchwin(line1_win);
    wrefresh(line1_win);
}



static	Item	msg_item;
static	Item	continue_item;
static	Item	yes_item;
static	Item	no_item;

static	int	continue_notify_proc();
static	int	confirm_notify_proc();


tty_setup_init()
{
    msg_item = TEXT_ITEM_OO(con_form, 5, 10, "", -1, 60);
    continue_item = TEXT_ITEM(con_form, 7, 10, "Press Return to continue.", -1, 10);
    form_item_set(continue_item, 
		  ITEM_DISPLAYED, 	FALSE, 
		  ITEM_NOTIFY_PROC, 	continue_notify_proc, 
		  ITEM_NOTIFY_LEVEL,	FORM_NOTIFY_ON_EACH_CHAR,
		  0);
    yes_item = BUTTON_ITEM(con_form, 8, 10, "Yes");
    form_item_set(yes_item, 
		  ITEM_DISPLAYED, 	FALSE, 
		  ITEM_NOTIFY_PROC, 	confirm_notify_proc, 
		  0);
    no_item = BUTTON_ITEM(con_form, 9, 10, "No");
    form_item_set(no_item, 
		  ITEM_DISPLAYED, 	FALSE, 
		  ITEM_NOTIFY_PROC, 	confirm_notify_proc, 
		  0);
}


tty_continue_proc(msg)
char	*msg;
{   
    Form	forms[2];
    
    form_item_set(msg_item, ITEM_VALUE, msg, 0);
    form_item_set(msg_item, ITEM_DISPLAYED, TRUE, 0);
    form_item_set(continue_item, ITEM_DISPLAYED, TRUE, 0);
    form_set(con_form, 
	     FORM_CURRENT_IO_ITEM, 	continue_item, 
	     FORM_DISPLAY, 		TRUE,
	     0);
    forms[0] = error_form;
    forms[1] = con_form;
    form_process_input(forms, 2, 1);
    form_item_set(msg_item, ITEM_DISPLAYED, FALSE, 0);
    form_item_display(msg_item);
    form_item_set(continue_item, ITEM_DISPLAYED, FALSE, 0);    
    form_item_display(continue_item);
    form_set(con_form, FORM_REFRESH, 0);
}


static
int
continue_notify_proc(item, ch)
Item	item;
int	ch;
{
    if ((char)ch == '\n') 
    	return(TRUE);
    else
	return(FALSE);
}
    
		  

static	int	confirm_val;

tty_confirm_proc(msg)
char	*msg;
{   
    Form	forms[2];
    
    form_item_set(msg_item, ITEM_VALUE, msg, 0);
    form_item_set(msg_item, ITEM_DISPLAYED, TRUE, 0);
    form_item_set(yes_item, ITEM_DISPLAYED, TRUE, 0);
    form_item_set(no_item, ITEM_DISPLAYED, TRUE, 0);
    form_set(con_form, 
	     FORM_CURRENT_IO_ITEM, 	yes_item, 
	     FORM_DISPLAY, 		TRUE,
	     0);
    forms[0] = error_form;
    forms[1] = con_form;
    form_process_input(forms, 2, 1);
    form_item_set(msg_item, ITEM_DISPLAYED, FALSE, 0);
    form_item_display(msg_item);
    form_item_set(yes_item, ITEM_DISPLAYED, FALSE, 0); 
    form_item_display(yes_item);
    form_item_set(no_item, ITEM_DISPLAYED, FALSE, 0);
    form_item_display(no_item);
    form_set(con_form, FORM_REFRESH, 0);    
    return(confirm_val);
}

static
int 
confirm_notify_proc(item, ch)
Item	item;
int	ch;
{   
    if (item == yes_item)
	confirm_val = TRUE;
    else
	confirm_val = FALSE;
    
    return(TRUE);
}
    


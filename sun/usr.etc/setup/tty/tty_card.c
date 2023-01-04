#ifndef lint
static	char sccsid[] = "@(#)tty_card.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
* Copyright (c) 1985 by Sun Microsystems, Inc.
*/



#include "tty_global.h"
#include "tty_item.h"
#include "tty_list.h"

static	Item	delete_item;
static	Item	edit_item;
static	Item	close_item;
static	Item	default_item;
static	Item	apply_to_item;

static	Item	name_item;
static	Item	cpu_type_item;
static	Item	root_disk_item;
static	Item	root_size_item;
static	Item	swap_disk_item;
static	Item	swap_size_item;
static	Item	three_com_item;

static	List	card_list;
static	int	edit_card_notify_proc();
static	int	default_card_notify_proc();
static	int	delete_card_notify_proc();
static	int	close_card_notify_proc();
static	int	new_name_notify_proc();
static	int	draw_lines();


extern	Form	card_form;
extern	Form	card_list_form;

extern	Workstation ws;




tty_card_init() 
{   
    register	int	i;
    register	int	card_index;
    		Card	card;
    
    form_set(card_form, FORM_DISPLAY_FUNC, draw_lines, 0);

    edit_item = TEXT_ITEM(card_form, 1, 0, "Edit Card:", 16, 12);
    form_set(card_form, FORM_CURRENT_IO_ITEM, edit_item, 0); 
    form_item_set(edit_item, 
		  ITEM_NOTIFY_PROC,	edit_card_notify_proc, 
		  0);
    
    
    default_item = TEXT_ITEM(card_form, 2, 0, "Default Card:", -1, 10 );
    form_item_set(default_item, 
		  ITEM_NOTIFY_PROC,	default_card_notify_proc, 
		  ITEM_VALUE, 		
		   ((setup_get(ws, WS_DEFAULT_CARD) == NULL) ? "" :
		       (setup_get(setup_get(ws, WS_DEFAULT_CARD), CLIENT_NAME))), 
		  0);
    
    
    close_item = BUTTON_ITEM(card_form, 1, (CARD_WIN_WIDTH - 9), "Close");
    form_item_set(close_item, 
		  ITEM_NOTIFY_PROC,	close_card_notify_proc, 
		  0);
    
    delete_item = BUTTON_ITEM(card_form, 2, (CARD_WIN_WIDTH - 10), "Delete");
    form_item_set(delete_item, 
		  ITEM_NOTIFY_PROC,	delete_card_notify_proc, 
		  0);
    
    
    name_item = TEXT_ITEM(card_form, 5, 0, "Card Name:", 15, 15);
    cpu_type_item = 
	CHOICE_ITEM(card_form, 6, 0, "CPU type:", 15);
    glue_choices(cpu_type_item, ws, CONFIG_CARD_CPU);
    form_item_set(cpu_type_item, ITEM_CHOICE_CYCLE_DISPLAY, TRUE, 0);
    
    root_disk_item = CHOICE_ITEM(card_form, 7, 0, "Root Disk:", 15);
    form_item_set(root_disk_item, ITEM_CHOICE_CYCLE_DISPLAY, TRUE, 0);
    root_size_item = TEXT_ITEM(card_form, 8, 0, "Root Size:", 15, 15);
    swap_disk_item = CHOICE_ITEM(card_form, 9, 0, "Swap Disk:", 15); 
    form_item_set(swap_disk_item, ITEM_CHOICE_CYCLE_DISPLAY, TRUE, 0);
    swap_size_item = TEXT_ITEM(card_form, 10, 0, "Swap Size:", 15, 15);
    three_com_item = CHOICE_ITEM(card_form, 11, 0, "3com Board", 15);
    form_item_set(three_com_item, 
		  ITEM_CHOICE_STRING,	"No", 
		  ITEM_CHOICE_STRING,	"Yes", 
		  0);
    
    apply_to_item = TEXT_ITEM(card_form, 13, 0, "Apply Card to:", -1, 15);
    
    /* 
     * Get the list of Sun supplied default cards.
     */
    card_list = list_create(card_list_form, 10, 0, CARD_WIN_WIDTH);
    
    SETUP_FOREACH_OBJECT(ws, WS_CARD, card_index, card) {
	list_add(card_list, (char *)setup_get(card, CLIENT_NAME));
    } SETUP_END_FOREACH;
    display_card_info(FALSE);
    
}

tty_card_pre_display()
{   
    
    tty_get_card_nd_names(root_disk_item);
    tty_get_card_nd_names(swap_disk_item);
}

tty_get_card_nd_names(item)
Item	item;
{   
    int			index;
    char		*str;
    
    form_item_set(item, ITEM_CHOICE_RESET, 0);
    SETUP_FOREACH_CHOICE(ws, CONFIG_CARD_ND, index, str) {
	form_item_set(item, ITEM_CHOICE_STRING, str, 0);
    } SETUP_END_FOREACH;
}


static
int
draw_lines(form)
Form	form;
{   
    extern	WINDOW	*client_win;
    extern	WINDOW	*card_win;
    register	int	i;
    		int	old_row, old_col;
		
   /* 
    * draw lines to differenicate the client and card windows.
    * 
    */
    
    getyx(client_win, old_row, old_col);
    wmove(client_win, 0, 0);
    for (i = 0; i < (CLIENT_WIN_WIDTH - 1) ; i++)
	waddch(client_win, '-');
    waddch(client_win, '+');
    for (i = 0; i < CARD_CLIENT_WIN_SIZE; i++) {
	wmove(client_win, (i + 1), (CLIENT_WIN_WIDTH - 1));	
	waddch(client_win, '|');
    }
    wmove(client_win, old_row, old_col);
    
    getyx(card_win, old_row, old_col);
    wmove(card_win, 0, 0);
    for (i = 0; i < CARD_WIN_WIDTH; i++)
	waddch(card_win, '-');
    wmove(card_win, old_row, old_col);
        
    wrefresh(client_win);
    wrefresh(card_win);
}


static	int	card_callback_proc();
static	int	fill_in_card_values();
static	int	fill_in_null_card_values();

static	Card	cur_card = NULL;

static
int	
edit_card_notify_proc(item, ch)
Item	item;
int	ch;
{   
    register	char		*name;
    extern	Workstation	ws;
    register	Card		card;
    extern	Cursor_control	cursor_control;
    extern	int		callback_error;
    
    name = (char *) form_item_get(item, ITEM_VALUE);
    if (strcmp(name, "") == 0) {
	if (cur_card != NULL) {
	    form_item_set(edit_item, 
			  ITEM_VALUE,  setup_get(cur_card, CLIENT_NAME), 0);
	    form_item_display(edit_item);
	    form_item_set(edit_item, ITEM_SET_CURSOR, 0);
	    cursor_control = CURSOR_CONTROL_STAY_PUT;
	}
    }
    else if (cur_card != NULL &&
      strcmp(name, setup_get(cur_card, CLIENT_NAME)) == 0) {
	return(FALSE);
    }
    else {
	card = setup_get(ws, WS_CARD_NAME, name);
	if (card == NULL) {    
	    card = setup_create(CARD, 
				SETUP_CALLBACK, 	card_callback_proc, 
				CLIENT_NAME, 	name, 
				0);
	    if (!callback_error) {
	    	/* created new card */
		setup_set(ws, WS_CARD, SETUP_APPEND, card, 0);
		undo_glue(cur_card);	/* undo bindings of previous card  */
		cur_card = card; 
		fill_in_card_values(cur_card);
		cursor_control = CURSOR_CONTROL_IGNORE;
	    }
	    else {
	    	/* could not create card; maybe its name was illegal */
		cursor_control = CURSOR_CONTROL_STAY_PUT;
		display_card_info(FALSE);
		setup_destroy(card, 0);
	    }
	}
	else {
	    undo_glue(cur_card);	/* undo bindings of previous card  */
	    cur_card = card;
	    fill_in_card_values(cur_card);
	    cursor_control = CURSOR_CONTROL_IGNORE;
	}
    }
    form_set(card_form, FORM_REFRESH, 0);    
    return(FALSE);
}

static
int
card_callback_proc(obj, attr, display_value, err_msg)
Opaque          obj;
Setup_attribute attr;
caddr_t         display_value;
char            *err_msg;
{   
    extern	int		callback_error;
    
    if (err_msg == NULL) {
	list_add(card_list, (char *)form_item_get(edit_item, ITEM_VALUE));
	callback_error = FALSE;
    }
    else {
	tty_error_msg(err_msg);
	callback_error = TRUE;
    }
    form_item_set(edit_item, ITEM_VALUE, display_value, 0);
    form_item_set(edit_item, 
		  ITEM_HILIGHTED, (int)setup_get(obj, SETUP_STATUS, attr), 
		  0);    
    form_item_display(edit_item);
}



static
int
delete_card_notify_proc(item, ch)
Item	item;
int	ch;
{   
    register	char		*name;
    extern	Workstation	ws;
    register	Card		card;
    extern	int		advance_to_next_item;    
    
    name = setup_get(cur_card, CLIENT_NAME);
    setup_destroy(ws, WS_CARD_NAME, name, 0);
    list_remove(card_list, name);
    display_card_info(FALSE);
    form_item_set(edit_item, ITEM_VALUE, "", 0);
    form_item_display(edit_item);

    if (strcmp(form_item_get(default_item, ITEM_VALUE), name) == 0) {
	/* deleting the default card */
	setup_set(ws, WS_DEFAULT_CARD, (Card)NULL, 0);
	form_item_set(default_item, ITEM_VALUE, "", 0);
	form_item_display(default_item);
    }
    
    form_set(card_form, FORM_REFRESH, 0);
    cur_card = (Card)NULL;
    return(FALSE);
}


static
int
close_card_notify_proc(item, ch)
Item	item;
int	ch;
{   
    display_card_info(FALSE);
    form_item_set(edit_item, ITEM_VALUE, "", 0);
    form_item_display(edit_item);
    form_set(card_form, FORM_REFRESH, 0);
    cur_card = (Card)NULL;
    return(FALSE);
}



static
int
fill_in_card_values()
{   
    extern	void	glue_callback_proc();
    
    setup_set(cur_card, SETUP_CALLBACK, glue_callback_proc, 0);
    glue(name_item, cur_card, CLIENT_NAME, (Item)NULL);
    form_item_set(name_item, ITEM_NOTIFY_PROC, new_name_notify_proc, 0);
    glue(apply_to_item, cur_card, CARD_APPLY_TO, (Item)NULL);
    
    glue(cpu_type_item,  cur_card, CLIENT_ARCH, 		 (Item)NULL);
    glue(root_disk_item, cur_card, CLIENT_ROOT_PARTITION_INDEX,  (Item)NULL);
    glue(root_size_item, cur_card, CLIENT_ROOT_SIZE_STRING_LEFT, (Item)NULL);
    glue(swap_disk_item, cur_card, CLIENT_SWAP_PARTITION_INDEX,  (Item)NULL);
    glue(swap_size_item, cur_card, CLIENT_SWAP_SIZE_STRING_LEFT, (Item)NULL);
    glue(three_com_item, cur_card, CLIENT_3COM_INTERFACE,	 (Item)NULL);
    display_card_info(TRUE);
    setup_set(cur_card, SETUP_CALLBACK, glue_callback_proc, 0);
    
    form_set(card_form, FORM_CURRENT_IO_ITEM, cpu_type_item, 0);
    form_item_set(cpu_type_item, ITEM_SET_CURSOR, 0);
}

static
int
display_card_info(val)
int	val;
{   
    form_item_set(apply_to_item, ITEM_DISPLAYED, val, 0);
    form_item_display(apply_to_item);
    form_item_set(name_item, ITEM_DISPLAYED, val, 0);
    form_item_display(name_item);
    form_item_set(cpu_type_item, ITEM_DISPLAYED, val, 0);
    form_item_display(cpu_type_item);
    form_item_set(root_disk_item, ITEM_DISPLAYED, val, 0);
    form_item_display(root_disk_item);
    form_item_set(root_size_item, ITEM_DISPLAYED, val, 0);
    form_item_display(root_size_item);
    form_item_set(swap_disk_item, ITEM_DISPLAYED, val, 0);
    form_item_display(swap_disk_item);
    form_item_set(swap_size_item, ITEM_DISPLAYED, val, 0);
    form_item_display(swap_size_item);
    form_item_set(three_com_item, ITEM_DISPLAYED, val, 0);
    form_item_display(three_com_item);
    form_item_set(delete_item, ITEM_DISPLAYED, val, 0);
    form_item_display(delete_item);
    form_item_set(close_item, ITEM_DISPLAYED, val, 0);
    form_item_display(close_item);
/****
    form_item_set(default_item, ITEM_DISPLAYED, val, 0);
    form_item_display(default_item);
*****/
}



static
int
new_name_notify_proc(item, ch)
Item	item;
int	ch;
{   
    char	old_name[256];
    char	*new_name;
    strcpy(old_name, setup_get(cur_card, CLIENT_NAME));
    
    glue_notify_proc(item, ch);
    new_name = setup_get(cur_card, CLIENT_NAME);
    if (strcmp(old_name, new_name) != 0) {
	/* old and new name are different, so the card name is the new
	 * name. 
	 */
	/* new name is OK */
	list_remove(card_list, old_name);
	list_add(card_list, new_name);
	form_item_set(edit_item, ITEM_VALUE, new_name, 0);
	form_item_display(edit_item);
	form_set(card_form, FORM_REFRESH, 0);    
    }
    return(FALSE);
}



static
int
undo_glue()
{   
    if (cur_card == NULL)
	return;
    
    setup_set(cur_card, SETUP_OPAQUE, CARD_APPLY_TO, 		   (Item)NULL,0);
    setup_set(cur_card, SETUP_OPAQUE, CLIENT_NAME, 		   (Item)NULL,0);
    setup_set(cur_card, SETUP_OPAQUE, CLIENT_ARCH,		   (Item)NULL,0);
    setup_set(cur_card, SETUP_OPAQUE, CLIENT_ROOT_PARTITION_INDEX, (Item)NULL,0);
    setup_set(cur_card, SETUP_OPAQUE, CLIENT_ROOT_SIZE_STRING_LEFT,(Item)NULL,0);
    setup_set(cur_card, SETUP_OPAQUE, CLIENT_SWAP_PARTITION_INDEX, (Item)NULL,0);
    setup_set(cur_card, SETUP_OPAQUE, CLIENT_SWAP_SIZE_STRING_LEFT,(Item)NULL,0);
    setup_set(cur_card, SETUP_OPAQUE, CLIENT_3COM_INTERFACE,	   (Item)NULL,0);
    
}



static
int
default_card_notify_proc(item, ch)
Item	item;
int	ch;
{   
    char	*card_name;
    Card	card;
    
    card_name = (char *) form_item_get(item, ITEM_VALUE);
    
    if (strcmp(card_name, "") == 0) {
	setup_set(ws, WS_DEFAULT_CARD, (Card)NULL, 0);
    }
    else {
	card = (Card) setup_get(ws, WS_CARD_NAME_TO_DEFAULT, card_name);
	if (card != (Card)NULL) {
	    setup_set(ws, WS_DEFAULT_CARD, card, 0);
        } else {
	    form_item_set(item, ITEM_VALUE, "", 0);
	    form_item_display(item);
	}
    }

    return(FALSE);
}

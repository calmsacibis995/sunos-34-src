#ifndef lint
static	char sccsid[] = "@(#)tty_client.c 1.1 86/09/25 Copyr 1985 Sun Micro";
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

static	Item	default_label_item;
static	Item	default_card_item;

static	Item	name_item;
static	Item	host_number_item;
static	Item	cpu_type_item;
static	Item	root_disk_item;
static	Item	root_size_item;
static	Item	swap_disk_item;
static	Item	swap_size_item;
static	Item	e_addr_item;
static	Item	three_com_item;

static	List	client_list;

extern	Form	client_form;
extern	Form	client_list_form;

extern	Workstation ws;


static	int	edit_client_notify_proc();
static	int	delete_client_notify_proc();
static	int	close_client_notify_proc();
static	int	new_name_notify_proc();
static	int	undo_glue();
static	int	draw_client_list_lines();

tty_client_init()
{   
    register	int	client_index;
    register	Client	client;
    
    form_set(client_list_form, FORM_DISPLAY_FUNC, draw_client_list_lines, 0);
        
    edit_item = TEXT_ITEM(client_form, 1, 0, "Edit Client:", -1, 15);
    form_set(client_form, FORM_CURRENT_IO_ITEM, edit_item, 0); 
    form_item_set(edit_item, ITEM_NOTIFY_PROC, edit_client_notify_proc, 0);
    
    close_item = BUTTON_ITEM(client_form, 1, (CLIENT_WIN_WIDTH - 9), "Close");
    form_item_set(close_item, ITEM_NOTIFY_PROC, close_client_notify_proc, 0);
    
    delete_item = BUTTON_ITEM(client_form, 2,(CLIENT_WIN_WIDTH - 10), "Delete");
    form_item_set(delete_item, ITEM_NOTIFY_PROC, delete_client_notify_proc, 0);
    
    name_item = TEXT_ITEM(client_form, 5, 0, "Client Name:", 18, 15);
    form_item_set(name_item, ITEM_NOTIFY_PROC, new_name_notify_proc, 0);
    
    cpu_type_item = CHOICE_ITEM(client_form, 6, 0, "CPU type:", 18);
    glue_choices(cpu_type_item, ws, CONFIG_CPU);
    form_item_set(cpu_type_item, ITEM_CHOICE_CYCLE_DISPLAY, TRUE, 0);
    
    root_disk_item = CHOICE_ITEM(client_form, 7, 0, "Root Disk:", 18);
    form_item_set(root_disk_item, ITEM_CHOICE_CYCLE_DISPLAY, TRUE, 0);
  
    root_size_item = TEXT_ITEM(client_form, 8, 0, "Root Size:", 18, 15);
    swap_disk_item = CHOICE_ITEM(client_form, 9, 0, "Swap Disk:", 18); 
    form_item_set(swap_disk_item, ITEM_CHOICE_CYCLE_DISPLAY, TRUE, 0);
    
    swap_size_item = TEXT_ITEM(client_form, 10, 0, "Swap Size:", 18, 15);
    
    three_com_item = CHOICE_ITEM(client_form, 11, 0, "3com Board:", 18);
    form_item_set(three_com_item, 
		  ITEM_CHOICE_STRING,	"No", 
		  ITEM_CHOICE_STRING,	"Yes", 
		  0);
    
    e_addr_item = 
	TEXT_ITEM(client_form, 12, 0, "Ethernet Addrs:", 18, 15);
    
    host_number_item = 
	TEXT_ITEM(client_form, 13, 0, "Host Number:", 18, 15);
    
    client_list = list_create(client_list_form, 12, 0, (CLIENT_WIN_WIDTH - 1));
    SETUP_FOREACH_OBJECT(ws, WS_CLIENT, client_index, client) {
	list_add(client_list, (char *)setup_get(client, CLIENT_NAME));
    } SETUP_END_FOREACH;
	
    display_client_info(FALSE);
}
	


static	int	client_callback_proc();
static	int	fill_in_client_values();
static	int	fill_in_null_client_values();

static	Client	cur_client = NULL;


static
int	
edit_client_notify_proc(item, ch)
Item	item;
int	ch;
{   
    register	char		*name;
    extern	Workstation	ws;
    register	Client		client;
    extern	Cursor_control	cursor_control;
    		char		buff[100];
    extern	int		callback_error;

    name = (char *) form_item_get(item, ITEM_VALUE);
    if (strcmp(name, "") == 0) {
	if (cur_client != NULL) {
	    form_item_set(edit_item, 
			  ITEM_VALUE,  setup_get(cur_client, CLIENT_NAME), 0);
	    form_item_display(edit_item);
	    form_item_set(edit_item, ITEM_SET_CURSOR, 0);
	    cursor_control = CURSOR_CONTROL_STAY_PUT;
	}
    }
    else if (cur_client != NULL && 
      strcmp(name, setup_get(cur_client, CLIENT_NAME)) == 0) {
	return(FALSE);
    }
    else {
	client = setup_get(ws, WS_CLIENT_NAME, name);
	if (client == NULL) {    
	    client = setup_create(CLIENT, 
				  SETUP_CALLBACK, 	client_callback_proc, 
				  CLIENT_NAME, 	name, 
				  0);
	    if (!callback_error) {
	    	/* created new client */
		setup_set(ws, WS_CLIENT, SETUP_APPEND, client, 0);
		undo_glue();	/* undo bindings of previous client  */
		cur_client = client; 
		fill_in_client_values();
		cursor_control = CURSOR_CONTROL_IGNORE;
	    }
	    else {
	    	/* could not create client; maybe its name was illegal */
		setup_destroy(client, 0);
		display_client_info(FALSE);
		cursor_control = CURSOR_CONTROL_STAY_PUT;
	    }
	}
	else {
	    undo_glue();	/* undo bindings of previous client  */
	    cur_client = client;
	    fill_in_client_values();
	    cursor_control = CURSOR_CONTROL_IGNORE;
	}
    }
    form_set(client_form, FORM_REFRESH, 0);    
    return(FALSE);
}

static
int
client_callback_proc(obj, attr, display_value, err_msg)
Opaque          obj;
Setup_attribute attr;
caddr_t         display_value;
char            *err_msg;
{   
    extern	List		client_list;
    extern	int		callback_error;
    
    if (err_msg == NULL) {
	list_add(client_list, (char *)form_item_get(edit_item, ITEM_VALUE));
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
delete_client_notify_proc(item, ch)
Item	item;
int	ch;
{   
    register	char		*name;
    extern	Workstation	ws;
    register	Client		client;
    extern	List		client_list;
    		char		buff[100];
    
    name = setup_get(cur_client, CLIENT_NAME);
    setup_destroy(ws, WS_CLIENT_NAME, name, 0);
    list_remove(client_list, name);
    display_client_info(FALSE);
    form_item_set(edit_item, ITEM_VALUE, "", 0);
    form_item_display(edit_item);
    form_set(client_form, FORM_REFRESH, 0);
    cur_client = (Client) NULL;
    return(FALSE);
}


static
int
close_client_notify_proc(item, ch)
Item	item;
int	ch;
{   
    display_client_info(FALSE);
    form_item_set(edit_item, ITEM_VALUE, "", 0);
    form_item_display(edit_item);
    form_set(client_form, FORM_REFRESH, 0);        
    cur_client = (Client) NULL;
    return(FALSE);
}



static
int
fill_in_client_values()
{   
    extern	void	glue_callback_proc();
    register	Item	item;
    
    setup_set(cur_client, SETUP_CALLBACK, glue_callback_proc, 0);
    glue(name_item, cur_client, CLIENT_NAME, (Item)NULL);
    form_item_set(name_item, ITEM_NOTIFY_PROC, new_name_notify_proc, 0);
    glue(e_addr_item, cur_client, CLIENT_E_ADDR, 		(Item)NULL);
    glue(host_number_item, cur_client, CLIENT_HOST_NUMBER, 	(Item)NULL);
    glue(cpu_type_item,  cur_client, CLIENT_ARCH,		(Item)NULL);
    glue(root_disk_item, cur_client, CLIENT_ROOT_PARTITION_INDEX,(Item)NULL);
    glue(root_size_item, cur_client, CLIENT_ROOT_SIZE_STRING_LEFT,(Item)NULL);
    glue(swap_disk_item, cur_client, CLIENT_SWAP_PARTITION_INDEX,(Item)NULL);
    glue(swap_size_item, cur_client, CLIENT_SWAP_SIZE_STRING_LEFT,(Item)NULL);
    glue(three_com_item, cur_client, CLIENT_3COM_INTERFACE,	(Item)NULL);
    display_client_info(TRUE);
    setup_set(cur_client, SETUP_CALLBACK, glue_callback_proc, 0);
    
    item = (Item)NULL;
    if ((int)setup_get(cur_client, SETUP_STATUS, CLIENT_NAME))
	item = name_item;
    else if ((int)setup_get(cur_client, SETUP_STATUS, CLIENT_ARCH))
	item = cpu_type_item;
    else if ((int)setup_get(cur_client, SETUP_STATUS, CLIENT_ROOT_PARTITION_INDEX))
	item = root_disk_item;
    else if ((int)setup_get(cur_client, SETUP_STATUS, CLIENT_ROOT_SIZE_STRING_LEFT))
	item = root_size_item;
    else if ((int)setup_get(cur_client, SETUP_STATUS, CLIENT_SWAP_PARTITION_INDEX))
	item = swap_disk_item;
    else if ((int)setup_get(cur_client, SETUP_STATUS, CLIENT_SWAP_SIZE_STRING_LEFT))
	item = swap_size_item;
    else if ((int)setup_get(cur_client, SETUP_STATUS, CLIENT_3COM_INTERFACE))
	item = three_com_item;
    else if ((int)setup_get(cur_client, SETUP_STATUS, CLIENT_E_ADDR))
	item = e_addr_item;
    else if ((int)setup_get(cur_client, SETUP_STATUS, CLIENT_HOST_NUMBER))
	item = host_number_item;
    else
	item = name_item;
    form_set(client_form, FORM_CURRENT_IO_ITEM, item, 0);
    form_item_set(item, ITEM_SET_CURSOR, 0);
}

static
int
display_client_info(val)
int	val;
{   
    form_item_set(name_item, ITEM_DISPLAYED, val, 0);
    form_item_display(name_item);
    form_item_set(e_addr_item, ITEM_DISPLAYED, val, 0);
    form_item_display(e_addr_item);
    form_item_set(host_number_item, ITEM_DISPLAYED, val, 0);
    form_item_display(host_number_item);
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
}



static
int
new_name_notify_proc(item, ch)
Item	item;
int	ch;
{   
    char	old_name[256];
    char	*new_name;
    strcpy(old_name, setup_get(cur_client, CLIENT_NAME));
    
    glue_notify_proc(item, ch);
    new_name = setup_get(cur_client, CLIENT_NAME);
    if (strcmp(old_name, new_name) != 0) {
	/* old and new name are different, so the client name is the new
	 * name. 
	 */
	/* new name is OK */
	list_remove(client_list, old_name);
	list_add(client_list, new_name);
	form_item_set(edit_item, ITEM_VALUE, new_name, 0);
	form_item_display(edit_item);
	form_set(client_form, FORM_REFRESH, 0);    
    }
    return(FALSE);
}




tty_get_nd_names(item)
Item	item;
{   
    int			hp_index;
    Hard_partition	hp;
    
    form_item_set(item, ITEM_CHOICE_RESET, 0);
    SETUP_FOREACH_OBJECT(ws, WS_ND_PARTITION, hp_index, hp) {
	form_item_set(item, ITEM_CHOICE_STRING, 
		      (char *)setup_get(hp, HARD_NAME), 
		      0);
    } SETUP_END_FOREACH;
}



tty_client_pre_display()
{   
    tty_get_nd_names(root_disk_item);
    tty_get_nd_names(swap_disk_item);
    tty_card_pre_display();
}



static
int
draw_client_list_lines(form)
Form	form;
{   
    extern	WINDOW	*client_list_win;
    register	int	i;
    int	old_row, old_col;
    
   /* 
    * draw vertical line for client list window
    * 
    */	
    getyx(client_list_win, old_row, old_col);
    for (i = 0; i < LIST_WIN_SIZE; i++) {
	wmove(client_list_win, i,(CLIENT_WIN_WIDTH - 1));
	waddch(client_list_win, '|');
    }
    wmove(client_list_win, old_row, old_col);
    
    wrefresh(client_list_win);
}



static
int
undo_glue()
{   
    if (cur_client == NULL)
	return;
    
    setup_set(cur_client, SETUP_OPAQUE, CLIENT_NAME, 		 (Item)NULL, 0);
    setup_set(cur_client, SETUP_OPAQUE, CLIENT_E_ADDR,		 (Item)NULL, 0);
    setup_set(cur_client, SETUP_OPAQUE, CLIENT_HOST_NUMBER,	 (Item)NULL, 0);
    setup_set(cur_client, SETUP_OPAQUE, CLIENT_MODEL,		 (Item)NULL, 0);
    setup_set(cur_client, SETUP_OPAQUE, CLIENT_ARCH,		 (Item)NULL, 0);
    setup_set(cur_client, SETUP_OPAQUE, 
	      CLIENT_ROOT_PARTITION_INDEX, (Item)NULL, 0);
    setup_set(cur_client, SETUP_OPAQUE, 
	      CLIENT_ROOT_SIZE_STRING_LEFT,(Item)NULL, 0);
    setup_set(cur_client, SETUP_OPAQUE, 
	      CLIENT_SWAP_PARTITION_INDEX, (Item)NULL, 0);
    setup_set(cur_client, SETUP_OPAQUE, 
	      CLIENT_SWAP_SIZE_STRING_LEFT,(Item)NULL, 0);
    setup_set(cur_client, SETUP_OPAQUE, 
	      CLIENT_3COM_INTERFACE, (Item)NULL, 0);
    
}


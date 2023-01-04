#ifndef lint
static	char sccsid[] = "@(#)tty_navigate.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "tty_global.h"
#include "tty_item.h"

extern	Form	nav_form;
extern	Form	error_form;
extern	Form	ws_form;
extern	Form	edit_disk_form;
extern	Form	software_form;
extern	Form	setup_form;
extern	Form	defaults_form;

extern	Form	client_list_form;
extern	Form	card_list_form;
extern	Form	client_form;
extern	Form	card_form;

extern	char	setup_msgbuf[];

typedef	struct {
    int		n_forms;
    Form	**form_ptrs;
    int		cur_form_index;
    int		(*func)();
}forms_info;

/* -------------------------------------------------------------------------- */
static	Item	ws_item;
static	Form	*ws_forms[] = {
    &ws_form
};
forms_info	ws_form_info = {
    (sizeof(ws_forms) / sizeof(Form *)), 
    ws_forms, 
    0, 
    NULL
};
/* -------------------------------------------------------------------------- */
static	Item	disk_item;
extern	int	tty_edit_disk_pre_display();
static	Form	*disk_forms[] = {
    &edit_disk_form
};
forms_info	disk_form_info = {
    (sizeof(disk_forms) / sizeof(Form *)), 
    disk_forms, 
    0, 
    tty_edit_disk_pre_display
};

/* -------------------------------------------------------------------------- */
static	Item	client_item;
static	Form	*client_forms[] = {
    &client_list_form, &card_list_form, 
    &client_form, &card_form
};
extern	int	tty_client_pre_display();
forms_info	client_form_info = {
    (sizeof(client_forms) / sizeof(Form *)), 
    client_forms, 
    2, 
    tty_client_pre_display
};

/* -------------------------------------------------------------------------- */
static	Item	software_item;
static	Form	*software_forms[] = {
    &software_form
};

extern	int	tty_software_pre_display();
forms_info	software_form_info = {
    (sizeof(software_forms) / sizeof(Form *)), 
    software_forms, 
    0, 
    tty_software_pre_display
};

/* -------------------------------------------------------------------------- */
static	Item	setup_item;
static	Form	*setup_forms[] = {
    &error_form
};

forms_info	setup_form_info = {
    (sizeof(setup_forms) / sizeof(Form *)),     
    setup_forms, 
    0, 
    NULL
};



/* -------------------------------------------------------------------------- */

static	Item	defaults_item;
static	Form	*defaults_forms[] = {
    &defaults_form
};

forms_info	defaults_form_info = {
    (sizeof(defaults_forms) / sizeof(Form *)), 
    defaults_forms, 
    0, 
    NULL
};



/* -------------------------------------------------------------------------- */


static	Item	execute_item;
static	Item	reboot_item;
static	Item	exit_item;


/* -------------------------------------------------------------------------- */

static	int	nav_notify_proc();
static	void	hilight_button();


tty_nav_init()
{   
    
    ws_item = BUTTON_ITEM(nav_form, 0, 0, "Workstation");
    form_item_set(ws_item, 
		  ITEM_CLIENT_DATA, 	&ws_form_info, 
		  ITEM_NOTIFY_PROC, 	nav_notify_proc, 
		  ITEM_HILIGHTED, 	TRUE, 
		  0);
    
    form_set(nav_form, FORM_CURRENT_IO_ITEM, ws_item, 0);
    
    
    client_item = BUTTON_ITEM(nav_form, 0, 20, "Clients");
    form_item_set(client_item, 
		  ITEM_CLIENT_DATA, 	&client_form_info, 
		  ITEM_NOTIFY_PROC, 	nav_notify_proc, 
		  0);
    
    software_item = BUTTON_ITEM(nav_form, 0, 40, "Software");
    form_item_set(software_item, 
		  ITEM_CLIENT_DATA, 	&software_form_info, 
		  ITEM_NOTIFY_PROC, 	nav_notify_proc, 
		  0);
    
    disk_item = BUTTON_ITEM(nav_form, 0, 60, "Disks");
    form_item_set(disk_item, 
		  ITEM_CLIENT_DATA, 	&disk_form_info, 
		  ITEM_NOTIFY_PROC, 	nav_notify_proc, 
		  0);
    
    defaults_item = BUTTON_ITEM(nav_form, 1, 0, "Defaults");
    form_item_set(defaults_item, 
 		  ITEM_CLIENT_DATA, 	&defaults_form_info,
		  ITEM_NOTIFY_PROC, 	nav_notify_proc, 
		  0);
    
    execute_item = BUTTON_ITEM(nav_form, 1, 20, "Execute-Setup");
    form_item_set(execute_item, 
		  ITEM_CLIENT_DATA, 	&setup_form_info, 
		  ITEM_NOTIFY_PROC, 	nav_notify_proc, 
		  0);
    
    reboot_item = BUTTON_ITEM(nav_form, 1, 40, "Reboot");
    form_item_set(reboot_item, 
		  ITEM_CLIENT_DATA, 	&setup_form_info, 
		  ITEM_NOTIFY_PROC, 	nav_notify_proc, 
		  0);
    
    exit_item = BUTTON_ITEM(nav_form, 1, 60, "Quit");
    form_item_set(exit_item, 
		  ITEM_NOTIFY_PROC, 	nav_notify_proc, 
		  0);
    
}


static	forms_info	*new_forms;
static	Item		cur_item;

tty_nav_doit()
{   
    register	int	num_forms;
    register	int	num_forms_to_replace;
    register	int	i;
    		Form	forms[MAX_NUM_FORMS];
		int	cur_form;
		
    /* 
     * these forms are always displayed.
     */
    forms[0] = nav_form;
    forms[1] = error_form;
    
    /* this form gets covered by others */
    forms[2] = ws_form;
    num_forms_to_replace = 1;
    num_forms = 3;
    cur_form = 2;
    form_set(forms[0], FORM_DISPLAY, TRUE, 0);
    form_set(forms[1], FORM_DISPLAY, TRUE, 0);
    form_set(forms[2], FORM_DISPLAY, TRUE, 0);
    cur_item = ws_item;
    
    while (1) {
	form_process_input(forms, num_forms, cur_form);
	num_forms -= num_forms_to_replace;
	for (i = 0; i < num_forms_to_replace; i++) {
	    if (forms[num_forms + i] != error_form)
		form_set(forms[num_forms + i], FORM_DISPLAY, FALSE, 0);
	}
	    
	cur_form = num_forms + new_forms->cur_form_index;
	if (new_forms->func != NULL)
	    (*new_forms->func)();
	for (i = 0; i < new_forms->n_forms; i++) {
	    forms[num_forms] = *(new_forms->form_ptrs[i]);
	    form_set(forms[num_forms], FORM_DISPLAY, TRUE, 0);
	    num_forms++;
	}
	num_forms_to_replace = new_forms->n_forms;
    }
}



static	int		pressed_special_button = FALSE;

static
int
nav_notify_proc(item, ch)
Item	item;
int	ch;
{   
    extern	Workstation	ws;
    extern	Cursor_control	cursor_control;
    extern	WINDOW		*error_win;
    extern	WINDOW		*line2_win;
    extern	Form		error_form;
    
    if (item == exit_item) {
	mid_cleanup();
	move(0, 0);
	clear();
	refresh();
	endwin();
	exit(0);
    }

    if ((item == client_item) &&
	((Workstation_type) setup_get(ws, WS_TYPE) != WORKSTATION_SERVER)) {
	new_forms = (forms_info *) form_item_get(cur_item, ITEM_CLIENT_DATA);
	runtime_message(SETUP_ENOTSERVER);
	tty_error_msg(setup_msgbuf);
	cursor_control = CURSOR_CONTROL_STAY_PUT;
	return(TRUE);
    }
    else if ((item == client_item) &&
	     ((int) setup_get(ws, WS_ETHERTYPE) == 0)) {
	new_forms = (forms_info *) form_item_get(cur_item, ITEM_CLIENT_DATA);
	tty_error_msg("Clients cannot be allocated unless you indicate your Ethernet Board.");
	cursor_control = CURSOR_CONTROL_STAY_PUT;
	return(TRUE);
    }
    else if (((item == software_item) || (item == disk_item) || (item == execute_item)) &&
	     ((Workstation_type) setup_get(ws, WS_TYPE) == WORKSTATION_NONE)) {
	new_forms = (forms_info *) form_item_get(cur_item, ITEM_CLIENT_DATA);
	/* runtime_message(SETUP_E???); */
	tty_error_msg("You must choose a workstation type before visting this form.");
	cursor_control = CURSOR_CONTROL_STAY_PUT;
	return(TRUE);
    }
    
    if (item == execute_item) {
	tty_setup_cmd(TTY_CMD_SETUP_EXECUTE);
	cursor_control = CURSOR_CONTROL_STAY_PUT;
	pressed_special_button = TRUE;
    }
    else if (item == reboot_item) {
	tty_setup_cmd(TTY_CMD_SETUP_REBOOT);
	cursor_control = CURSOR_CONTROL_STAY_PUT;
	pressed_special_button = TRUE;
    }
    else if (pressed_special_button) {
	pressed_special_button = FALSE;
	form_set(error_form, 
		 FORM_CURSES_WINDOW, 	error_win, 
		 FORM_FIRST_ROW, 	0, 
		 FORM_WIN_SIZE, 	ERROR_WIN_SIZE, 
		 0);
	tty_error_display_last_msg();
	touchwin(line2_win);
	wrefresh(line2_win);
    }
	    
    new_forms = (forms_info *) form_item_get(item, ITEM_CLIENT_DATA);    
    hilight_button(cur_item, FALSE);
    cur_item = item;
    hilight_button(cur_item, TRUE);
    form_set(nav_form, FORM_REFRESH, 0);
    
    return(TRUE);
}


static
void
hilight_button(item, val)
Item	item;
int	val;
{   
    form_item_set(item, ITEM_HILIGHTED, val, 0);
    form_item_display(item);
}
    

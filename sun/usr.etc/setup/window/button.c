
#ifndef lint
static	char sccsid[] = "@(#)button.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup.h"
#include <signal.h>

typedef void	(*Function)();


/* button that indicates the current
 * screen, and its normal position.
 */
static int		screen_value	= 0;

#define	BUTTON_HEIGHT	64		/* height of the button panel */

static void		navigate();
static void		execute();
static void		do_reboot();
static void		exit_setup();

/* screen drawing procs */
extern void		machine_show();
extern void		defaults_show();
extern void		client_show();
extern void		software_show();
extern void		disk_show();

/* array of screen drawing procs (start at 1) */
Function	screen_procs[] = {0, machine_show, defaults_show,
			client_show, software_show, disk_show
};

/* runtime message buffer */
extern  char    	setup_msgbuf[];

/* Initialize the button panel */
void
button_init()
{
    Panel	panel;
    Rect	rect;

    int		tab	 = 70;
    int		center	 = display_rect.r_width / 2;
 
    /* create the panel subwindows */   
    rect = display_rect;
    rect.r_height = BUTTON_HEIGHT;

    panel = make_panel(rect);
    
    display_rect.r_top = rect_bottom(&rect) + 6;
    display_rect.r_height -= rect.r_height + 6;

    /* initialize the global images */
    image_init(panel);
    
	panel_create_item(panel, PANEL_CHOICE, 
		    PANEL_CHOICE_FONTS, image_font, 0,
		    PANEL_LABEL_X, 	20,
		    PANEL_LABEL_Y, 	ATTR_ROW(1),
		    /*
		     * If you add things here, add them to
		     * screen_procs[], also
		     */
		    PANEL_CHOICE_STRINGS, 
			"Workstation", "Defaults", "Clients", "Software",
			"Disks", 0,
		    PANEL_NOTIFY_PROC,	navigate,
		    PANEL_FEEDBACK, 	PANEL_MARKED,
		      0);

    panel_create_item(panel, PANEL_BUTTON,
    		      PANEL_LABEL_X, 	center,
    		      PANEL_LABEL_IMAGE, image_execute,
		      PANEL_NOTIFY_PROC, execute,
    		      0);

    panel_create_item(panel, PANEL_BUTTON,
    		      PANEL_LABEL_X, 	center + 2 * tab,
    		      PANEL_LABEL_IMAGE, image_reboot,
		      PANEL_NOTIFY_PROC, do_reboot,
    		      0);

    panel_create_item(panel, PANEL_BUTTON,
    		      PANEL_LABEL_X, 	center + 5 * tab,
    		      PANEL_LABEL_IMAGE, image_exit,
    		      PANEL_NOTIFY_PROC, exit_setup,
    		      0);
    		      
    /* install the panel */
    win_insert(win_get_fd(panel));
}


/* allow navigation to screens
 * contingent upon the setting of
 * workstation and ethernet type
 */
static void
navigate(item, value, event)
Panel_item	item;
Event		*event;
{
Workstation_type	ws_type = (Workstation_type) setup_get(ws, WS_TYPE);

    /* 
     * Values start at 1 not 0, so that no value can be 0.
     * Thus must inc value by 1 here.
     */
    value++;
    switch(ws_type) {
        case WORKSTATION_SERVER: 	/* access to any screen */
            if (value != CLIENT_SCREEN || 
                    setup_get(ws, WS_ETHERTYPE) != SETUP_NOETHERNET)
                show_screen(value, 0);
            else {
                runtime_message(SETUP_ENOETHER);   
                message_print(setup_msgbuf); 
            }      
        break;
        
        case WORKSTATION_STANDALONE: 	/* anything but the client screen */
            if (value != CLIENT_SCREEN)
                show_screen(value, 0);
            else {
                runtime_message(SETUP_ENOTSERVER);  
                message_print(setup_msgbuf); 
            }      
        break;
    
        case WORKSTATION_NONE: 		/* only ws and defaults screens */
            if (value == MACHINE_SCREEN || value == DEFAULTS_SCREEN)
                show_screen(value, 0);
            else {
                runtime_message(SETUP_ENOWSTYPE); 
                message_print(setup_msgbuf); 
            }      
        break;
    }        
}


static void
exit_setup(item, event)
Panel_item	item;
Event		*event;
{
	if (confirm_yes_no("Really Quit?")) {
	    /*
	    tool_done_with_no_confirm(tool);
	    confirm_destroy();
	    */
	    /* blow away */
	    mid_cleanup();
	    seln_yield_all();
	    /*
	     * if there is a non-zero console_fd then we are running from the
	     * miniroot, and we should blow away suntools on our way down
	     */
	    if ((int)setup_get(ws, WS_CONSOLE_FD)) {
	        kill(getppid(), SIGKILL);
	        exit(0);
            } else {
	        exit(0);
	    }
	}
}



/* hide the current screen, update the
 * navigation panel, and call the show proc for
 * show_button.
 */
void
show_screen(show_value, data)
int		show_value;
Opaque		data;
{
    Function	show_proc;

    /* hide the current screen */
    if (screen_value) {
	show_proc = screen_procs[screen_value];
        (*show_proc)(0, FALSE);
    }

    /* move the button to the current screen
     * slot.
     */
    screen_value = show_value;
    show_proc = screen_procs[screen_value];
    (*show_proc)(data, TRUE);

}

/*
 * Notify proc for the "EXECUTE-SETUP" button.
 * Call the backend to actually do the install.
 * ARGSUSED
 */
static void
execute(button, data)
Panel_item	button;
Opaque		data;
{
	if (confirm("Really execute Setup?", FALSE))
	    setup_execute(ws);
}

/*
 * Notify proc for the "REBOOT" button.
 * Call the backend to actually do the reboot.
 * ARGSUSED
 */
static void
do_reboot(button, data)
Panel_item	button;
Opaque		data;
{
	if (confirm("Really Reboot?", FALSE))
	    setup_reboot(ws);
}

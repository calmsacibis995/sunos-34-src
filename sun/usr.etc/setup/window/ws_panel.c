#ifndef lint
static	char sccsid[] = "@(#)ws_panel.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup.h"

static Panel		panel = 0;

static Panel_item	e_board_type, domain, network, host;
static Panel_item	yp_type, yp_master_name, yp_master_internet;
static Panel_item	ws_type, cpu_type_served, name;
static Panel_item	tape_device, tape_location, tape_server, tape_address;

static void		display_disk_screen();
static void		init_ws_panels();
static void		layout_disks();

static Pixrect		*disk_image();
static Pixrect		*controller_image();


/* Show the workstation screen */
void
machine_show(data, show_it)
Opaque	data;
int	show_it;
{
    if (!panel)
       init_ws_panels();

    show_panel(panel, show_it);
}


/* Initialize the workstation screen */
static void
init_ws_panels()
{
    Panel_item	model, cpu_type;

    int		indent = 20;
    int		value_offset = 210;
    int		text_y_delta = 4;
    int		center = display_rect.r_width / 2;
    
 
    panel = make_panel(display_rect);
    
    /* Workstation items */
    
    panel_create_item(panel, PANEL_TEXT,
    		      PANEL_LABEL_X, indent,
    		      PANEL_LABEL_Y, ATTR_ROW(1),
     		      PANEL_VALUE_X, value_offset,
    		      PANEL_VALUE_Y, ATTR_ROW(1) + text_y_delta,
    		      PANEL_LABEL_IMAGE, image_string("Workstation Name:"),
    		      PANEL_VALUE_DISPLAY_LENGTH, 15,
		      PANEL_CLIENT_DATA, object_info(ws, WS_NAME),
		      PANEL_NOTIFY_PROC, set_text_info,
    		      0);

    ws_type =
        panel_create_item(panel, PANEL_CHOICE,
    		          PANEL_LABEL_IMAGE, image_string("Workstation Type:"),
    		          PANEL_MENU_TITLE_STRING, "Workstation Type:",
		          PANEL_CLIENT_DATA, object_info(ws, WS_TYPE),
		          PANEL_NOTIFY_PROC, set_info,
    		          0);

    get_choices(ws, CONFIG_TYPE, ws_type);
    panel_set(ws_type, CYCLE_ATTRS(indent, ATTR_ROW(2), value_offset), 0);
 
    /* show this item only if the workstation is a server
     * (i.e. type == WORKSTATION_SERVER) 
     */
    cpu_type_served = 
        panel_create_item(panel, PANEL_TOGGLE,
    		          PANEL_LAYOUT, PANEL_VERTICAL,
    		          PANEL_LABEL_X, 2 * indent,
    		          PANEL_LABEL_Y, ATTR_ROW(3),
     		          PANEL_VALUE_X, 3 * indent,
    		          PANEL_VALUE_Y, ATTR_ROW(4),
    		          PANEL_LABEL_IMAGE, 
			      image_string("CPU-Types Served:"),
    		          PANEL_MENU_TITLE_STRING, "CPU-Types Served:",
    		          PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
    		          PANEL_SHOW_ITEM,
    		              (Workstation_type)setup_get(ws, WS_TYPE) == 
				WORKSTATION_SERVER,
		          PANEL_CLIENT_DATA, object_info(ws, WS_ARCH_SERVED),
 		          PANEL_NOTIFY_PROC, set_info,
   		          0);
    		      
    get_choices(ws, CONFIG_CPU, cpu_type_served);
     
    /* Tape items */
    
    tape_device =
        panel_create_item(panel, PANEL_CHOICE,
    		          PANEL_LABEL_IMAGE, image_string("Tape Device:"),
    		          PANEL_MENU_TITLE_STRING, "Tape Device:",
		          PANEL_CLIENT_DATA, object_info(ws, WS_TAPE_TYPE),
		          PANEL_NOTIFY_PROC, set_info,
    		          0);

    get_choices(ws, CONFIG_TAPETYPE, tape_device);
    panel_set(tape_device, CYCLE_ATTRS(indent, ATTR_ROW(6), value_offset), 0);
     
    tape_location =
        panel_create_item(panel, PANEL_CHOICE,
    		          PANEL_LABEL_IMAGE, image_string("Tape Location:"),
    		          PANEL_MENU_TITLE_STRING, "Tape Location:",
		          PANEL_CLIENT_DATA, object_info(ws, WS_TAPE_LOC),
		          PANEL_NOTIFY_PROC, set_info,
    		          0);

    get_choices(ws, CONFIG_TAPE_LOCATION, tape_location);
    panel_set(tape_location, CYCLE_ATTRS(indent, ATTR_ROW(7), value_offset), 0);
     
    /* show these items only if the workstation has no tape
     * (i.e. tape_location == SETUP_REMOTE) 
     */
     
    tape_server = 
        panel_create_item(panel, PANEL_TEXT,
    		          PANEL_LABEL_X, 2 * indent,
    		          PANEL_LABEL_Y, ATTR_ROW(8),
     		          PANEL_VALUE_X, value_offset,
    		          PANEL_VALUE_Y, ATTR_ROW(8) + text_y_delta,
    		          PANEL_LABEL_IMAGE, image_string("Server Name:"),
    		          PANEL_VALUE_DISPLAY_LENGTH, 15,
    		          PANEL_SHOW_ITEM,
    		              (int) setup_get(ws, WS_TAPE_LOC) == 1,	/* remote */
		          PANEL_CLIENT_DATA, object_info(ws, WS_TAPE_SERVER),
		          PANEL_NOTIFY_PROC, set_text_info,
    		          0);
    tape_address =
        panel_create_item(panel, PANEL_TEXT,
    		          PANEL_LABEL_X, 2 * indent,
    		          PANEL_LABEL_Y, ATTR_ROW(9),
     		          PANEL_VALUE_X, value_offset,
    		          PANEL_VALUE_Y, ATTR_ROW(9) + text_y_delta,
    		          PANEL_LABEL_IMAGE, 
			      image_string("Server Internet #:"),
    		          PANEL_VALUE_DISPLAY_LENGTH, 15,
    		          PANEL_SHOW_ITEM,
    		              (int) setup_get(ws, WS_TAPE_LOC) == 1,	/* remote */
		          PANEL_CLIENT_DATA, 
			      object_info(ws, WS_HOST_INTERNET_NUMBER),
		          PANEL_NOTIFY_PROC, set_text_info,
    		          0);

    /* Network items */
     
    e_board_type =
        panel_create_item(panel, PANEL_CHOICE,
    		          PANEL_LABEL_IMAGE, image_string("Ethernet Interface:"),
    		          PANEL_MENU_TITLE_STRING, "Ethernet Interface:",
    		          PANEL_CLIENT_DATA, object_info(ws, WS_ETHERTYPE),
		          PANEL_NOTIFY_PROC, set_info,
   		          0);

    get_choices(ws, CONFIG_ETHERTYPE, e_board_type);
    panel_set(e_board_type, 
    	      CYCLE_ATTRS(center - indent, ATTR_ROW(1), value_offset),
    	      0);
     
    /* show these items only if the workstation is on the net
     * (i.e. type != SETUP_NOETHERNET) 
     */
   		      
    host = 
        panel_create_item(panel, PANEL_TEXT,
    		      PANEL_LABEL_X, center,
    		      PANEL_LABEL_Y, ATTR_ROW(2),
     		      PANEL_VALUE_X, center + value_offset - indent,
    		      PANEL_VALUE_Y, ATTR_ROW(2) + text_y_delta,
    		      PANEL_LABEL_IMAGE, image_string("Host Number:"),
    		      PANEL_VALUE_DISPLAY_LENGTH, 15,
		      PANEL_CLIENT_DATA, object_info(ws, WS_HOST_NUMBER),
    		      PANEL_SHOW_ITEM,
    		          setup_get(ws, WS_ETHERTYPE) != SETUP_NOETHERNET,
		      PANEL_NOTIFY_PROC, set_text_info,
    		      0);

    yp_type=
        panel_create_item(panel, PANEL_CHOICE,
    		      PANEL_LABEL_IMAGE, image_string("Yellow Pages Type:"),
    		      PANEL_MENU_TITLE_STRING, "Yellow Pages Type:",
    		      PANEL_CLIENT_DATA, object_info(ws, WS_YPTYPE),
		      PANEL_NOTIFY_PROC, set_info,
    		      PANEL_SHOW_ITEM,
			  setup_get(ws, WS_ETHERTYPE) != SETUP_NOETHERNET,
   		      0);

   get_choices(ws, CONFIG_YP_TYPE, yp_type);
   panel_set(yp_type,  
    	     CYCLE_ATTRS(center - indent, ATTR_ROW(4), value_offset),
    	     0);

    /* show these items only if the workstation cares about YP
     * (i.e. not(YP_NONE)) 
     */

    domain = 
        panel_create_item(panel, PANEL_TEXT,
    		      PANEL_LABEL_X, center,
    		      PANEL_LABEL_Y, ATTR_ROW(5),
     		      PANEL_VALUE_X, center + value_offset - indent,
   		      PANEL_VALUE_Y, ATTR_ROW(5) + text_y_delta,
    		      PANEL_LABEL_IMAGE, image_string("Domain:"),
    		      PANEL_VALUE_DISPLAY_LENGTH, 15,
		      PANEL_CLIENT_DATA, object_info(ws, WS_DOMAIN),
    		      PANEL_SHOW_ITEM,
    		          (Yp_type) setup_get(ws, WS_YPTYPE) != YP_NONE,
		      PANEL_NOTIFY_PROC, set_text_info,
    		      0);
    		      
    /* show these items only if the workstation is a YP_SLAVE_SERVER
     */
     
    yp_master_name = 
        panel_create_item(panel, PANEL_TEXT,
    		      PANEL_LABEL_X, center,
    		      PANEL_LABEL_Y, ATTR_ROW(6),
     		      PANEL_VALUE_X, center + value_offset - indent,
    		      PANEL_VALUE_Y, ATTR_ROW(6) + text_y_delta,
    		      PANEL_LABEL_IMAGE, image_string("Master Name:"),
    		      PANEL_VALUE_DISPLAY_LENGTH, 15,
		      PANEL_CLIENT_DATA, object_info(ws, WS_YPMASTER_NAME),
		      PANEL_NOTIFY_PROC, set_text_info,
    		      PANEL_SHOW_ITEM,
    		          (Yp_type) setup_get(ws, WS_YPTYPE) == YP_SLAVE_SERVER,
    		      0);
   

    yp_master_internet = 
        panel_create_item(panel, PANEL_TEXT,
    		      PANEL_LABEL_X, center,
    		      PANEL_LABEL_Y, ATTR_ROW(7),
     		      PANEL_VALUE_X, center + value_offset - indent,
    		      PANEL_VALUE_Y, ATTR_ROW(7) + text_y_delta,
    		      PANEL_LABEL_IMAGE, image_string("Master Internet #:"),
    		      PANEL_VALUE_DISPLAY_LENGTH, 15,
		      PANEL_CLIENT_DATA, object_info(ws, WS_YPMASTER_INTERNET),
		      PANEL_NOTIFY_PROC, set_text_info,
    		      PANEL_SHOW_ITEM,
    		          (Yp_type) setup_get(ws, WS_YPTYPE) == YP_SLAVE_SERVER,
    		      0);
 
    layout_disks();
}
   		          

/* Create the panel items for the controllers
 * and their disks.
 */
static void
layout_disks()
{
    Controller	cont;
    Disk	disk;
    int		cont_index, disk_index;
    int		start_y = 280;
    int		x, y, controller_space_y, disk_space_y;
    int		num_disks, num_controllers;
    int		center = display_rect.r_width / 2;  

    num_controllers = (int)setup_get(ws, WS_NUM_CONTROLLERS);
    
    /* compose vertically based on the 
     * number of controllers
     */
    if (num_controllers > 2) {
	controller_space_y = (display_rect.r_height) / 
		  ((num_controllers - 1) / 2 + 2);
	y = start_y;
    } else
        y = (display_rect.r_height) / 2;

    SETUP_FOREACH_OBJECT(ws, WS_CONTROLLER, cont_index, cont)
        if (cont_index % 2 == 0) {
            if ((num_controllers - cont_index) == 1)
    	        x = center - 70;	/* just one, center it */
            else
		x = center - 280;	/* first of two */
    	} else
	    x = center + 140;		/* second of two */
	        
    	panel_create_item(panel, PANEL_MESSAGE,
			  PANEL_LABEL_X, x,
			  PANEL_LABEL_Y, y,
			  PANEL_LABEL_IMAGE, controller_image(cont),
			  0);

        num_disks = (int)setup_get(cont, CONTROLLER_NUM_DISKS);
	disk_space_y = 95 - (num_disks * 15);
        x -= (num_disks - 1) * disk_space_y; 
        SETUP_FOREACH_OBJECT(cont, CONTROLLER_DISK, disk_index, disk)
            panel_create_item(panel, PANEL_MESSAGE,
			      PANEL_LABEL_X, x,
    		              PANEL_LABEL_Y, y + 75,
    			      PANEL_LABEL_IMAGE, disk_image(disk),
   		              PANEL_CLIENT_DATA, object_info(disk, SETUP_ALL),
   		           /*  used to be PANEL_BUTTON 
			      PANEL_NOTIFY_PROC, display_disk_screen,
			    */  
			      0);
	    x += (2 * disk_space_y);
	    setup_set(disk, SETUP_CALLBACK, handle_error, 0);
    	SETUP_END_FOREACH
    	if (cont_index % 2 == 1)
	    y += controller_space_y;	/* begin a new row */
     SETUP_END_FOREACH
}
 
  		          
void
update_ws_type(item, value)
Panel_item		item;
Workstation_type	value;
{
    panel_set(cpu_type_served, 
        PANEL_SHOW_ITEM, value == WORKSTATION_SERVER, 
        0);
}


void
update_tape_location(item, value)
Panel_item	item;
int		value;
{
    int	show_it = value == 1;	/* remote */

    panel_set(tape_server, PANEL_SHOW_ITEM, show_it, 0);
    panel_set(tape_address, PANEL_SHOW_ITEM, show_it, 0);
    
    if (show_it)
        set_caret(item, 0);
}


void
update_e_board_type(item, value)
Panel_item	item;
int		value;
{
    int	show_it = value != SETUP_NOETHERNET;

    panel_set(host, PANEL_SHOW_ITEM, show_it, 0);
    panel_set(yp_type, PANEL_SHOW_ITEM, show_it, 0);
    
    if (show_it)
        set_caret(item, 0);
    else { 		/* make sure yp subordinates are not shown */
        panel_set(domain, PANEL_SHOW_ITEM, show_it, 0);
        panel_set(yp_master_name, PANEL_SHOW_ITEM, show_it, 0);
        panel_set(yp_master_internet, PANEL_SHOW_ITEM, show_it, 0);
    }   
}


void
update_yp_type(item, value)
Panel_item	item;
Yp_type		value;
{
    int		show_it;

    show_it = value != YP_NONE;
    panel_set(domain, PANEL_SHOW_ITEM, show_it, 0);
    
    show_it = value == YP_SLAVE_SERVER;
    panel_set(yp_master_name, PANEL_SHOW_ITEM, show_it, 0);
    panel_set(yp_master_internet, PANEL_SHOW_ITEM, show_it, 0);
    
    set_caret(item, 0);
}


static void
display_disk_screen(item, event)
Panel_item	item;
Event		*event;
{
    Object_info	*info = (Object_info *) panel_get(item, PANEL_CLIENT_DATA);
    Workstation_type	ws_type = (Workstation_type) setup_get(ws, WS_TYPE);
    extern  char    	setup_msgbuf[];

    if (ws_type == WORKSTATION_NONE) {
	runtime_message(SETUP_EWS_NONE); 
	message_print(setup_msgbuf); 
    } else
	show_screen(DISK_SCREEN, info->obj);
}


static Pixrect *
controller_image(cont)
Controller	cont;
{
	switch ((Setup_setting)setup_get(cont, CONTROLLER_TYPE)) {

	    case SETUP_SCSI:
	        if ((int)setup_get(cont, CONTROLLER_NUM_DISKS) > 1) 
                    return(image_sd_controller2);
                else 
	            return(image_sd_controller1);

	    case SETUP_XYLOGICS:
	        if ((int)setup_get(cont, CONTROLLER_NUM_DISKS) > 1) 
                    return(image_xy_controller2);
                else 
	            return(image_xy_controller1);

	    default:
	        if ((int)setup_get(cont, CONTROLLER_NUM_DISKS) > 1) 
                    return(image_controller2);
                else 
	            return(image_controller1);
	}
}


static Pixrect *
disk_image(disk)
Disk	disk;
{
	switch ((Setup_setting)setup_get(disk, DISK_TYPE)) {

	case SETUP_SCSI: 
    	    return (image_label_pr(image_sd_disk, 
	                           setup_get(disk, DISK_NAME), 
				   -1));
	case SETUP_XYLOGICS: 
	    return (image_label_pr(image_xy_disk, 
	                           setup_get(disk, DISK_NAME), 
				   -1));
	default:
	    return (image_label_pr(image_disk, 
	                           setup_get(disk, DISK_NAME), 
				   -1));
	}      	    
}

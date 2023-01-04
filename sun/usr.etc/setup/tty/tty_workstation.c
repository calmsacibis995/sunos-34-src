#ifndef lint
static	char sccsid[] = "@(#)tty_workstation.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
* Copyright (c) 1985 by Sun Microsystems, Inc.
*/


#include "tty_global.h"
#include "tty_item.h"


static	Item	name_item;
static	Item	ws_type_item;
static	Item		arch_served_item;
static	Item	tape_device_item;
static	Item	tape_location_item;
static	Item		tape_server_name_item;
static	Item		tape_server_host_item;
static	Item	ether_type_item;
static	Item		host_number_item;
static	Item		yp_type_item;
static	Item			domain_item;
static	Item			yp_master_name_item;
static	Item			yp_master_inet_item;

static	int	tape_state();
static	int	ws_type_notify_proc();
static	int	draw_tape_info();
static	int	draw_yp_info();
static	int	yp_info_draw();
static	int	yp_type_notify_proc();
static	int	draw_net_info();
static	int	net_info_draw();
static	int	ether_board_notify_proc();

extern	Workstation ws;
extern	Form	ws_form;


tty_workstation_init()
{   
    register	int	displayed;
    
    
    name_item = TEXT_ITEM(ws_form, 0, 0, "Workstation Name:", 29, 20);
    glue(name_item, ws, WS_NAME, (Item)NULL);
    form_set(ws_form, FORM_CURRENT_IO_ITEM, name_item, 0);
    
    /* --------------- Workstation Type --------------------------- */
    
    ws_type_item = CHOICE_ITEM(ws_form, 2, 0, "Workstation Type:", 29);
    glue_choices(ws_type_item, ws, CONFIG_TYPE);
    arch_served_item = TOGGLE_ITEM(ws_form, 3, 4,"CPUs served:", 29);
    glue_toggle_strings(arch_served_item, ws, CONFIG_CPU);
    
    glue(ws_type_item, ws, WS_TYPE, (Item)NULL);
    form_item_set(ws_type_item, 
		  ITEM_NOTIFY_PROC, ws_type_notify_proc, 
		  0);
    glue(arch_served_item, ws, WS_ARCH_SERVED, (Item)NULL);
    form_item_set(arch_served_item, 
		  ITEM_DISPLAYED, (WORKSTATION_SERVER == 
				   (Workstation_type)setup_get(ws, WS_TYPE)),
		  0);
    
    /* ------------------ Tape stuff ------------------------*/
    
    tape_device_item = CHOICE_ITEM(ws_form, 5, 0, "Tape Device:", 29);
    glue_choices(tape_device_item, ws, CONFIG_TAPETYPE);
    glue(tape_device_item, ws, WS_TAPE_TYPE, (Item)NULL);
    form_item_set(tape_device_item, ITEM_CHOICE_CYCLE_DISPLAY, TRUE,  0);
    
    tape_location_item = CHOICE_ITEM(ws_form, 6, 0, "Tape Location:", 29);
    glue_choices(tape_location_item, ws, CONFIG_TAPE_LOCATION);
    glue(tape_location_item, ws, WS_TAPE_LOC, (Item)NULL);
    form_item_set(tape_location_item, 
		  ITEM_CHOICE_CYCLE_DISPLAY, TRUE, 
		  ITEM_NOTIFY_PROC, tape_state, 
		  0);
  
    
    tape_server_name_item = 
	TEXT_ITEM(ws_form, 7, 4, "Tape Server Name:", 29, 16);
    tape_server_host_item = 
	TEXT_ITEM(ws_form, 8, 4, "Internet Number:", 29,  16);
    glue(tape_server_name_item, ws, WS_TAPE_SERVER, (Item)NULL);
    glue(tape_server_host_item, ws, WS_HOST_INTERNET_NUMBER, (Item)NULL);
    draw_tape_info((int)setup_get(ws, WS_TAPE_LOC));
    
    
    /* ------------------ Network stuff ------------------ */
    ether_type_item = CHOICE_ITEM(ws_form, 10, 0, "Ethernet Interface:", 29);
    form_item_set(ether_type_item, 
		  ITEM_CHOICE_CYCLE_DISPLAY, TRUE, 
		  0);
    glue_choices(ether_type_item, ws, CONFIG_ETHERTYPE);
    glue(ether_type_item, ws, WS_ETHERTYPE, (Item)NULL);
    form_item_set(ether_type_item, 
		  ITEM_NOTIFY_PROC, ether_board_notify_proc, 
		  0);

    host_number_item = TEXT_ITEM(ws_form, 11, 4, "Host Number:", 29, 16);
    glue(host_number_item, ws, WS_HOST_NUMBER, (Item)NULL);
    
    yp_type_item = CHOICE_ITEM(ws_form, 12, 4, "YP type:", 29);
    glue(yp_type_item, ws, WS_YPTYPE, (Item)NULL);
    glue_choices(yp_type_item, ws, CONFIG_YP_TYPE);
    form_item_set(yp_type_item, 
		  ITEM_CHOICE_CYCLE_DISPLAY, 	TRUE, 
		  ITEM_NOTIFY_PROC, 		yp_type_notify_proc, 
		  0);

    
    domain_item = TEXT_ITEM(ws_form, 13, 8, "Domain:", 29, 16);        
    yp_master_name_item = TEXT_ITEM(ws_form, 14, 8, "Master Name:", 29, 20);    
    yp_master_inet_item = TEXT_ITEM(ws_form, 15, 8, "Master Internet #:", 29, 20);
    
    glue(domain_item, ws, WS_DOMAIN, (Item)NULL);    
    glue(yp_master_name_item, ws, WS_YPMASTER_NAME, (Item)NULL);
    glue(yp_master_inet_item, ws, WS_YPMASTER_INTERNET, (Item)NULL);
    
    net_info_draw();
}
    


determine_display_state(parent, child, grandchild)
Item	parent;
Item	child;
Item	grandchild;
{   
        
    register	int	val;
    
    val = (int) form_item_get(parent, ITEM_VALUE);
    form_item_set(child, ITEM_DISPLAYED, val, 0);
    form_item_display(child);
    if (grandchild) {
	if (val)
	    determine_display_state(child, grandchild, (Item)NULL);
	else {
	    form_item_set(grandchild, ITEM_DISPLAYED, FALSE, 0);
	    form_item_display(grandchild);
	}
    }
}


static
int
draw_tape_info(val)
register	int	val;
{   
    form_item_set(tape_server_name_item, ITEM_DISPLAYED, val, 0);
    form_item_set(tape_server_host_item, ITEM_DISPLAYED, val, 0);
    
    form_item_display(tape_server_name_item);
    form_item_display(tape_server_host_item);
}


static
int
tape_state(item, ch)
Item	item;
int	ch;
{   
    register	int	val;
    
    val = (int) form_item_get(item, ITEM_VALUE);
    if (val == 1 )  /* remote tape */
	draw_tape_info(TRUE);
    else
	draw_tape_info(FALSE);
    
    glue_notify_proc(item, ch);    
    return (FALSE);
}


static
int
draw_yp_info(domain_vis, master_vis)
register	int	domain_vis;
register	int	master_vis;
{   
    form_item_set(domain_item,		ITEM_DISPLAYED, domain_vis, 0);
    form_item_set(yp_master_name_item,	ITEM_DISPLAYED, master_vis, 0);
    form_item_set(yp_master_inet_item,	ITEM_DISPLAYED, master_vis, 0);
    
    form_item_display(domain_item);
    form_item_display(yp_master_name_item);
    form_item_display(yp_master_inet_item);
}

static
int
yp_info_draw()
{   
    register	Yp_type	val;
    
    val = (Yp_type) form_item_get(yp_type_item, ITEM_VALUE);
    
    if (val == YP_NONE) 
	draw_yp_info(FALSE, FALSE);
    else if ((val == YP_MASTER_SERVER) || (val == YP_CLIENT))
	draw_yp_info(TRUE, FALSE);
    else 
	draw_yp_info(TRUE, TRUE); /* YP_SLAVE_SERVER */
   
}

static
int
yp_type_notify_proc(item, ch)
Item	item;
int	ch;
{   
    yp_info_draw();
    glue_notify_proc(item, ch);        
    return (FALSE);
}



static
int
draw_net_info(vis)
register	int	vis;
{   
    form_item_set(yp_type_item, 	   ITEM_DISPLAYED, vis, 0);
    form_item_set(host_number_item, 	   ITEM_DISPLAYED, vis, 0);
    if (vis == TRUE)
	glue(host_number_item, ws, WS_HOST_NUMBER, (Item)NULL);
    
    form_item_display(yp_type_item);
    form_item_display(host_number_item);
}

static
int
net_info_draw()
{   
    register	int	val;
    
    val = (int) form_item_get(ether_type_item, ITEM_VALUE);
    if (val == 0) {
	/* no ethernet controller  cannot be on a network*/
	draw_net_info(FALSE);
	draw_yp_info(FALSE, FALSE);
    }
    else {
	draw_net_info(TRUE);
	yp_info_draw();
    }
}

static
int
ether_board_notify_proc(item, ch)
Item	item;
int	ch;
{   
    register	int	val;
    extern	int	callback_error;
    
    glue_notify_proc(item, ch);
    if (!callback_error)
	net_info_draw();
    return (FALSE);
}



static
int
ws_type_notify_proc(item, ch)
Item	item;
int	ch;
{   
    register	Workstation_type	val;
    
    val = (Workstation_type) form_item_get(item, ITEM_VALUE);
    if (val == WORKSTATION_SERVER) {
	form_item_set(arch_served_item, ITEM_DISPLAYED, TRUE, 0);
    }
    else {
	form_item_set(arch_served_item, ITEM_DISPLAYED, FALSE, 0);
    }
    form_item_display(arch_served_item);
    glue_notify_proc(item, ch);    
    return (FALSE);
}


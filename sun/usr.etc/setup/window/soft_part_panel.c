
#ifndef lint
static	char sccsid[] = "@(#)soft_part_panel.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup.h"

static Panel		soft_part_panel = 0;
static Panel_item	soft_client, soft_type, soft_size;

static void		init_soft_part_screen();
static void		layout_soft_parts();
static void		edit_soft_part();
static void		init_soft_part_items();

#define INDENT		 20
#define TAB		 70
 
/* Show or hide the soft partition screen */
void
soft_part_show(hp, show_it)
Hard_partition	hp;
short		show_it;
{
    if (!soft_part_panel) 
       init_soft_part_screen();
       
    /* if (hp) {
     *   Panel_item item += (Panel_item) setup_get(hp, SETUP_ALL);
     *   Object_info *info = (Object_info *) panel_get(item, PANEL_CLIENT_DATA);
     * }  
     */ 
          
    show_panel(soft_part_panel, show_it);
}

/* Initialize the soft partition screen */
static void
init_soft_part_screen()
{
    Panel_item		disk_item;
    Controller		cont;
    Disk		disk;
    Soft_partition	soft_part;
    Pixrect		*disk_image;
    int			cont_index, disk_index, soft_part_index;
    int 		x, row = 0;
    
    soft_part_panel 	= make_panel(display_rect);
  		      
    /* Partition icons */
    x = 150;
    SETUP_FOREACH_OBJECT(ws, WS_CONTROLLER, cont_index, cont)
        SETUP_FOREACH_OBJECT(cont, CONTROLLER_DISK, disk_index, disk)
            disk_item = setup_get(disk, SETUP_OPAQUE, SETUP_ALL);
	    disk_image = (Pixrect *) panel_get(disk_item, PANEL_LABEL_IMAGE);
            panel_create_item(soft_part_panel, PANEL_BUTTON,
			      PANEL_LABEL_X, x,
    		              PANEL_LABEL_Y, 5,
    			      PANEL_LABEL_IMAGE, disk_image,
   		              PANEL_CLIENT_DATA, object_info(disk, SETUP_ALL),
   		            /*
		  	      PANEL_NOTIFY_PROC, layout_soft_parts,
		  	    */	
			      0 );
	    x += TAB;
    	SETUP_END_FOREACH
    SETUP_END_FOREACH
    
    init_soft_part_items();
}

static void
layout_soft_parts(hp, x)
Hard_partition		hp;
int			x;
{
#ifdef notdef
    Soft_partition	sp;
    int			sp_index;
    int			row = 1;
    
    SETUP_FOREACH_OBJECT(hp, HARD_SOFT_PARTITION, sp_index, sp)
        panel_create_item(soft_part_panel, PANEL_BUTTON,
			  PANEL_LABEL_X, x,
    		          PANEL_LABEL_Y, ATTR_ROW(row + sp_index),
    			  PANEL_LABEL_IMAGE, 
    			      image_label_pr(image_hardpart,
    		                             setup_get(sp, SOFT_NAME),
    		                             2),  
   		          PANEL_CLIENT_DATA, object_info(sp, SETUP_ALL),
		  	  PANEL_NOTIFY_PROC, edit_soft_part,
			  0 );
    SETUP_END_FOREACH
#endif notdef
}

static void
init_soft_part_items()
{
    panel_create_item(soft_part_panel, PANEL_MESSAGE,
    		      PANEL_LABEL_X, INDENT,
    		      PANEL_LABEL_Y, ATTR_ROW(2),
    		      PANEL_LABEL_STRING, "SOFT",
    		      PANEL_LABEL_BOLD, TRUE,
    		      PANEL_SHOW_ITEM, TRUE,
    		      0);
    		      
    panel_create_item(soft_part_panel, PANEL_MESSAGE,
    		      PANEL_LABEL_X, INDENT,
    		      PANEL_LABEL_Y, ATTR_ROW(3),
    		      PANEL_LABEL_STRING, "PARTITION",
    		      PANEL_LABEL_BOLD, TRUE,
    		      PANEL_SHOW_ITEM, TRUE,
    		      0);
   
    soft_client = panel_create_item(soft_part_panel, PANEL_TEXT,
    		      PANEL_LABEL_X, INDENT,
    		      PANEL_LABEL_Y, ATTR_ROW(5),
    		      PANEL_LABEL_STRING, "Client:",
    		      PANEL_VALUE_DISPLAY_LENGTH, 10,
    		      PANEL_CLIENT_DATA, object_info(0, SOFT_CLIENT),
    		      PANEL_SHOW_ITEM, TRUE,
    		      0);
   
    soft_size = panel_create_item(soft_part_panel, PANEL_TEXT,
    		      PANEL_LABEL_X, INDENT,
    		      PANEL_LABEL_Y, ATTR_ROW(6),
    		      PANEL_LABEL_STRING, "Size:",
    		      PANEL_VALUE_DISPLAY_LENGTH, 15,
    		      PANEL_CLIENT_DATA, object_info(0, SOFT_SIZE),
    		      PANEL_SHOW_ITEM, TRUE,
    		      0);
 
    soft_type = panel_create_item(soft_part_panel, PANEL_CHOICE,
    		      PANEL_LABEL_X, INDENT,
    		      PANEL_LABEL_Y, ATTR_ROW(7),
    		      PANEL_LABEL_STRING, "Type:",
    		      PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		      PANEL_CLIENT_DATA, object_info(0, SOFT_TYPE),
    		      PANEL_SHOW_ITEM, TRUE,
    		      0);
    /*		          
      get_choices(soft_part, SOFT_TYPE, soft_type);
     */
}


static void
edit_soft_part(item, event)
Panel_item	item;
Event		*event;
{
    register Soft_partition	sp;
    register Object_info	*info;

    info = (Object_info *) panel_get(item, PANEL_CLIENT_DATA);
    sp = info->obj;

    panel_set(soft_client, PANEL_LABEL_STRING, setup_get(sp, SOFT_CLIENT), 
	      PANEL_SHOW_ITEM, TRUE,
	      0);
  
    info = (Object_info *) panel_get(soft_type, PANEL_CLIENT_DATA);
    info->obj = sp;
    panel_set(soft_type, PANEL_VALUE, setup_get(sp, HARD_TYPE),
	      PANEL_SHOW_ITEM, TRUE,
	      0);
    
    
    info = (Object_info *) panel_get(soft_size, PANEL_CLIENT_DATA);
    info->obj = sp;
    panel_set(soft_size, PANEL_VALUE, setup_get(sp, HARD_SIZE),
	      PANEL_SHOW_ITEM, TRUE,
	      0);
}



#ifndef lint
static	char sccsid[] = "@(#)client_panel.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup.h"

static Panel		client_panel 		= 0,
			card_panel 		= 0,
			client_item_panel 	= 0,
			card_item_panel 	= 0;
			
static Rect		client_panel_rect, card_panel_rect,
			client_item_panel_rect, card_item_panel_rect;

static Panel_item	client_item 		= 0,
			card_item 		= 0,
			default_card_name, default_card_image,
			edit_client_item, edit_card_item,
			dance_client, dance_card;

typedef enum {
    ROW_NAME,
    ROW_ARCH,
    ROW_ROOT_DISK,
    ROW_ROOT_SIZE,
    ROW_SWAP_DISK,
    ROW_SWAP_SIZE,
    ROW_3COM,
    ROW_EADDR,
    ROW_HOST,
    ROW_LAST
} Row;

typedef struct {
   char			*label;
   Setup_attribute	attr;
} Row_info;

static Row_info	row_info[ord(ROW_LAST)] = {
    "Name:",		CLIENT_NAME,
    "CPU Type:",	CLIENT_ARCH,
    "Root Partition:",	CLIENT_ROOT_PARTITION_INDEX,
    "Root Size:",	CLIENT_ROOT_SIZE_STRING_LEFT,
    "Swap Partition:",	CLIENT_SWAP_PARTITION_INDEX,
    "Swap Size:",	CLIENT_SWAP_SIZE_STRING_LEFT,
    "3COM Ethernet:",	CLIENT_3COM_INTERFACE,
    "Ethernet Addr:",	CLIENT_E_ADDR,
    "Host Number:",	CLIENT_HOST_NUMBER,
};

#define	FOREACH_ROW(row)	\
    for (row = 0; row < ord(ROW_LAST); row++) {

#define	END_FOREACH	}


static Panel_item	client_items[ord(ROW_LAST)];
static Panel_item	card_items[ord(ROW_LAST)];


static Panel_item	client_close, client_delete, client_edit;

static Panel_item	card_close, card_delete, card_edit,
			card_make_default, card_clear_default, card_apply_to;


static void		init_client_screen();

static void		init_client_items();
static void		show_client_info();
static void		client_selected();
static void		toggle_client();
static void		edit_client();
static void		close_client();
static void		add_client();
static void		delete_client();
static void		assign_client_position();
static void		get_nd_partitions();
static Panel_setting	set_client_name();

static void		init_card_items();
static void		show_card_info();
static void		card_selected();
static void		toggle_card();
static void		edit_card();
static void		close_card();
static void		add_card();
static void		delete_card();
static void		assign_card_position();
static void		make_default_card();
static Panel_setting	set_card_name();
static Panel_setting	set_default_card();

#define INDENT		 20
#define VALUE_OFFSET	 180
#define TEXT_Y_DELTA	 5
#define TAB		 70
#define OPEN_X		 164
#define OPEN_Y		 20
 
/* Show the client screen */
void
client_show(client, show_it)
Client	client;
short	show_it;
{
    if (!client_panel) 
       init_client_screen();
       
    /* get the latest nd partition
     * choices.
     */
    get_nd_partitions();

    show_panel(client_panel, show_it);
    show_panel(card_panel, show_it);
    show_panel(client_item_panel, show_it);
    show_panel(card_item_panel, show_it);
}

/* Initialize the client screen */
static void
init_client_screen()
{
    Panel_item	disk_item;
    Controller	cont;
    Disk	disk;
    Client	client;
    Card	card, default_card;
    Pixrect	*disk_image;
    int		cont_index, disk_index, client_index;
    int 	x, max_x, row = 0;
    
    int		client_panel_height, card_panel_height;
    int		client_panel_width, card_panel_width;
    int		client_item_panel_height, card_item_panel_height;
    int		client_item_panel_width, card_item_panel_width;

    client_item_panel_height = display_rect.r_height/2;
    client_panel_height = display_rect.r_height - client_item_panel_height - 5;
    
    client_panel_width = (display_rect.r_width) / 2;
    card_panel_width = display_rect.r_width - client_panel_width - 5;
                 
    rect_construct(&client_panel_rect, 
                   	5, 
                   	display_rect.r_top,
                   	client_panel_width, 
                   	client_panel_height);
                                        
    rect_construct(&card_panel_rect, 
    			rect_right(&client_panel_rect) + 6, 
                   	display_rect.r_top,
                   	card_panel_width, 
                   	client_panel_height);
                   
    rect_construct(&client_item_panel_rect, 
                   	5, 
                   	rect_bottom(&client_panel_rect) + 6,
                   	client_panel_width, 
                   	client_item_panel_height);
                                        
    rect_construct(&card_item_panel_rect,
                   	rect_right(&client_item_panel_rect) + 6, 
                   	rect_bottom(&card_panel_rect) + 6,
                   	card_panel_width, 
                   	client_item_panel_height);
                                         
    client_panel 	= make_panel(client_panel_rect);
    card_panel 		= make_panel(card_panel_rect);
    client_item_panel 	= make_panel(client_item_panel_rect);
    card_item_panel 	= make_panel(card_item_panel_rect);
    
    panel_set(client_panel, PANEL_VERTICAL_SCROLLBAR, scrollbar_build(0), 0);
    panel_set(card_panel, PANEL_VERTICAL_SCROLLBAR, scrollbar_build(0), 0);
      

    /* Client icons */
    SETUP_FOREACH_OBJECT(ws, WS_CLIENT, client_index, client)
	assign_client_position(&x, &row);
        panel_create_item(client_panel, PANEL_BUTTON,
			  PANEL_LABEL_X, x,
    		          PANEL_LABEL_Y, row,
    			  PANEL_LABEL_IMAGE, 
    			      image_label_pr(image_client,
    			                     setup_get(client, CLIENT_NAME),
    			                     -1),
   		          PANEL_CLIENT_DATA, 
   		              object_info(client, SETUP_ALL),
		  	  PANEL_NOTIFY_PROC, client_selected,	
			  0 );
	setup_set(client, SETUP_CALLBACK, handle_error, 0);		  
    SETUP_END_FOREACH

    /* Card icons */
    SETUP_FOREACH_OBJECT(ws, WS_CARD, client_index, card)
	assign_card_position(&x, &row);
        panel_create_item(card_panel, PANEL_BUTTON,
			  PANEL_LABEL_X, x,
    		          PANEL_LABEL_Y, row,
    			  PANEL_LABEL_IMAGE, 
    			      image_label_pr(image_card,
    			                     setup_get(card, CLIENT_NAME),
    			                     -1),
   		          PANEL_CLIENT_DATA, 
   		              object_info(card, SETUP_ALL),
		  	  PANEL_NOTIFY_PROC, card_selected,	
			  0 );
	setup_set(card, SETUP_CALLBACK, handle_error, 0);		  
    SETUP_END_FOREACH
    
    		      
    init_card_items();
    init_client_items();
}


static void
init_client_items()
{
    register int	row;
    Panel_item		item;

    edit_client_item = 
	panel_create_item(client_item_panel, PANEL_BUTTON,
            PANEL_ITEM_Y, OPEN_Y, 
	    PANEL_CLIENT_DATA, object_info(0, SETUP_ALL),
	    PANEL_NOTIFY_PROC, client_selected,
	    0);

    dance_client = 
	panel_create_item(client_item_panel, PANEL_MESSAGE,
                          PANEL_ITEM_Y, OPEN_Y, 
               		  0);

    FOREACH_ROW(row)
	switch (row) {
	    case ROW_NAME:
                item = panel_create_item(client_item_panel, PANEL_TEXT,
		  	   PANEL_LABEL_IMAGE, image_string("Client Name:"),
                           PANEL_VALUE_X, VALUE_OFFSET,
                           PANEL_VALUE_Y, ATTR_ROW(row + 4) + TEXT_Y_DELTA,
                           PANEL_VALUE_DISPLAY_LENGTH, 15,
                           PANEL_NOTIFY_PROC, set_client_name,
                           0);
		break;
		
	    case ROW_EADDR:
	    case ROW_HOST:
	    case ROW_ROOT_SIZE:
	    case ROW_SWAP_SIZE:
		/* text items */
                item = panel_create_item(client_item_panel, PANEL_TEXT,
		  	   PANEL_LABEL_IMAGE, image_string(row_info[row].label),
                           PANEL_VALUE_X, VALUE_OFFSET,
                           PANEL_VALUE_Y, ATTR_ROW(row + 4) + TEXT_Y_DELTA,
                           PANEL_VALUE_DISPLAY_LENGTH, 15,
                           PANEL_NOTIFY_PROC, set_text_info,
                           0);
		break;
   
	    default:
		/* choice items */
                item = panel_create_item(client_item_panel, PANEL_CHOICE,
		  	   PANEL_LABEL_IMAGE, image_string(row_info[row].label),
                           CYCLE_ATTRS(INDENT, ATTR_ROW(row + 4), VALUE_OFFSET),
                           PANEL_MENU_TITLE_STRING, row_info[row].label,
                           PANEL_NOTIFY_PROC, set_info,
                           0);
		switch (row) {
		    case ROW_ARCH:
                        get_choices(ws, CONFIG_CPU, item);
                        panel_set(item,
                            CYCLE_ATTRS(INDENT, ATTR_ROW(row + 4), 
                           	        VALUE_OFFSET - INDENT),
                            PANEL_PAINT, PANEL_NONE,
                            0);
			break;
		    case ROW_3COM:
                        panel_set(item,
                            PANEL_CHOICE_STRINGS,  "no", "yes", 0,
                            CYCLE_ATTRS(INDENT, ATTR_ROW(row + 4), 
                           	        VALUE_OFFSET - INDENT),
                            PANEL_PAINT, PANEL_NONE,
                            0);
			break;
		    default:
			break;
		}
		break;
	}
	panel_set(item, 
		  PANEL_CLIENT_DATA, object_info(0, row_info[row].attr),
                  PANEL_LABEL_X, INDENT,
                  PANEL_LABEL_Y, ATTR_ROW(row + 4),
		  PANEL_SHOW_ITEM, FALSE,
		  PANEL_PAINT, PANEL_NONE,
		  0);
	client_items[row] = item;
    END_FOREACH
    
   
    client_edit = panel_create_item(client_item_panel, PANEL_TEXT,
                      PANEL_LABEL_X, 300,
                      PANEL_LABEL_Y, ATTR_ROW(4),
                      PANEL_LABEL_IMAGE, image_string("Edit Client:"),
                      PANEL_VALUE_X, 410,
                      PANEL_VALUE_Y, ATTR_ROW(4) + TEXT_Y_DELTA,
                      PANEL_VALUE_DISPLAY_LENGTH, 10,
                      PANEL_NOTIFY_PROC, edit_client,
                      0);
   

    client_close = 
        panel_create_item(client_item_panel, PANEL_BUTTON,
                      PANEL_LABEL_X, 300,
                      PANEL_LABEL_Y, ATTR_ROW(5),
                      PANEL_LABEL_IMAGE, image_close,
                      PANEL_NOTIFY_PROC, close_client,
                      PANEL_SHOW_ITEM, FALSE,
                      0);
                      
    client_delete = 
        panel_create_item(client_item_panel, PANEL_BUTTON,
                      PANEL_LABEL_X, 300,
                      PANEL_LABEL_Y, ATTR_ROW(6),
                      PANEL_LABEL_IMAGE, image_delete,
                      PANEL_NOTIFY_PROC, delete_client,
                      PANEL_SHOW_ITEM, FALSE,
                      0);
                      
    default_card_image = 
	panel_create_item(client_item_panel, PANEL_MESSAGE, 
                          PANEL_ITEM_X, 350,
                          PANEL_ITEM_Y, ATTR_ROW(8), 
    			  PANEL_LABEL_IMAGE,
    			      image_label_pr(image_card,
    			          setup_get(
    			              (Card) setup_get(ws, WS_DEFAULT_CARD), 
    			              CLIENT_NAME),
    			          -1),
               		  0);
               		  
    default_card_name = panel_create_item(client_item_panel, PANEL_TEXT,
                      PANEL_LABEL_X, 300,
                      PANEL_LABEL_Y, ATTR_ROW(11),
                      PANEL_LABEL_IMAGE, image_string("Default Card:"),
                      PANEL_VALUE_X, 410,
                      PANEL_VALUE_Y, ATTR_ROW(11) + TEXT_Y_DELTA,
                      PANEL_VALUE_DISPLAY_LENGTH, 10,
                      PANEL_NOTIFY_PROC, set_default_card,
                      0);
    panel_set_value(default_card_name,
                    (char *) setup_get((Card) setup_get(ws, WS_DEFAULT_CARD), 
                                       CLIENT_NAME)
                    );                  
 }


static void
init_card_items()
{
    register int	row;
    Panel_item		item;

    edit_card_item = 
	panel_create_item(card_item_panel, PANEL_BUTTON,
            PANEL_ITEM_Y, OPEN_Y, 
	    PANEL_CLIENT_DATA, object_info(0, SETUP_ALL),
	    PANEL_NOTIFY_PROC, card_selected,
	    0);

    dance_card = panel_create_item(card_item_panel, PANEL_BUTTON,
                                            PANEL_ITEM_Y, OPEN_Y,
                                            0);

    card_edit = panel_create_item(card_item_panel, PANEL_TEXT,
                      PANEL_LABEL_X, 300,
                      PANEL_LABEL_Y, ATTR_ROW(4),
                      PANEL_LABEL_IMAGE, image_string("Edit Card:"),
                      PANEL_VALUE_X, 410,
                      PANEL_VALUE_Y, ATTR_ROW(4) + TEXT_Y_DELTA,
                      PANEL_VALUE_DISPLAY_LENGTH, 10,
                      PANEL_NOTIFY_PROC, edit_card,
                      0);
   
    FOREACH_ROW(row)
	switch (row) {
	    case ROW_EADDR:
	    case ROW_HOST:
	        item = (Panel_item) 0;
                break;

	    case ROW_NAME:
                item = panel_create_item(card_item_panel, PANEL_TEXT,
		     	   PANEL_LABEL_IMAGE, image_string("Card Name:"),
                           PANEL_VALUE_X, VALUE_OFFSET,
                           PANEL_VALUE_Y, ATTR_ROW(row + 4) + TEXT_Y_DELTA,
                           PANEL_VALUE_DISPLAY_LENGTH, 15,
                           PANEL_NOTIFY_PROC, set_card_name,
                           0);
		break;
		
	    case ROW_ROOT_SIZE:
	    case ROW_SWAP_SIZE:
		/* text items */
                item = panel_create_item(card_item_panel, PANEL_TEXT,
		     	   PANEL_LABEL_IMAGE, image_string(row_info[row].label),
                           PANEL_VALUE_X, VALUE_OFFSET,
                           PANEL_VALUE_Y, ATTR_ROW(row + 4) + TEXT_Y_DELTA,
                           PANEL_VALUE_DISPLAY_LENGTH, 15,
                           PANEL_NOTIFY_PROC, set_text_info,
                           0);
		break;
   
	    default:
		/* choice items */
                item = panel_create_item(card_item_panel, PANEL_CHOICE,
		      	   PANEL_LABEL_IMAGE, image_string(row_info[row].label),
                           CYCLE_ATTRS(INDENT, ATTR_ROW(row + 4), VALUE_OFFSET),
                           PANEL_MENU_TITLE_STRING, row_info[row].label,
                           PANEL_NOTIFY_PROC, set_info,
                           0);
		switch (row) {
		    case ROW_ARCH:
                        get_choices(ws, CONFIG_CARD_CPU, item);
                        panel_set(item,
                            CYCLE_ATTRS(INDENT, ATTR_ROW(row + 4), 
                           	        VALUE_OFFSET - INDENT),
		  	    PANEL_PAINT, PANEL_NONE,
                            0);
			break;
		    case ROW_3COM:
                        panel_set(item,
                            PANEL_CHOICE_STRINGS,  "no", "yes", 0,
                            CYCLE_ATTRS(INDENT, ATTR_ROW(row + 4), 
                           	        VALUE_OFFSET - INDENT),
		  	    PANEL_PAINT, PANEL_NONE,
                            0);
			break;
		    default:
			break;
		}
		break;
	}
        if (item)
	    panel_set(item, 
		      PANEL_CLIENT_DATA, object_info(0, row_info[row].attr),
		      PANEL_LABEL_X, INDENT,
		      PANEL_LABEL_Y, ATTR_ROW(row + 4),
		      PANEL_SHOW_ITEM, FALSE,
		      PANEL_PAINT, PANEL_NONE,
		      0);
	card_items[row] = item;
    END_FOREACH
    
    card_close = 
        panel_create_item(card_item_panel, PANEL_BUTTON,
                      PANEL_LABEL_X, 300,
                      PANEL_LABEL_Y, ATTR_ROW(5),
                      PANEL_LABEL_IMAGE, image_close,
                      PANEL_NOTIFY_PROC, close_card,
                      PANEL_SHOW_ITEM, FALSE,
                      0);
 
    card_delete =
	panel_create_item(card_item_panel, PANEL_BUTTON,
			  PANEL_LABEL_X, 300,
			  PANEL_LABEL_Y, ATTR_ROW(6),
			  PANEL_LABEL_IMAGE, image_delete,
			  PANEL_NOTIFY_PROC, delete_card,
			  PANEL_SHOW_ITEM, FALSE,
			  0);
 
    card_make_default =
	panel_create_item(card_item_panel, PANEL_BUTTON,
			  PANEL_LABEL_X, 300,
			  PANEL_LABEL_Y, ATTR_ROW(11),
			  PANEL_LABEL_IMAGE, 
                              panel_button_image(card_item_panel, 
                              			"Make Default", 0, image_font), 
                          PANEL_NOTIFY_PROC, make_default_card,
			  PANEL_SHOW_ITEM, FALSE,
			  0);
			  
    card_clear_default =
	panel_create_item(card_item_panel, PANEL_BUTTON,
			  PANEL_LABEL_X, 300,
			  PANEL_LABEL_Y, ATTR_ROW(11),
			  PANEL_LABEL_IMAGE, 
                              panel_button_image(card_item_panel, 
					      "Clear Default", 0, image_font), 
                          PANEL_NOTIFY_PROC, make_default_card,
			  PANEL_SHOW_ITEM, TRUE,
			  0);
			  
    card_apply_to =
	panel_create_item(card_item_panel, PANEL_TEXT,
			  PANEL_LABEL_X, 300,
			  PANEL_LABEL_Y, ATTR_ROW(8),
			  PANEL_LABEL_STRING, "Apply Card to:",
			  PANEL_CLIENT_DATA, object_info(0, CARD_APPLY_TO),
			  PANEL_SHOW_ITEM, FALSE,
			  PANEL_NOTIFY_PROC, set_text_info,
			  0);
   
}


static void
edit_client(item, event)
Panel_item	item;
Event		*event;
{
    Client	client = setup_get(ws, WS_CLIENT_NAME, panel_get_value(item));
            
    if (!client)
        add_client(item,event);
    else
        toggle_client(client);
}


static void
edit_card(item, event)
Panel_item	item;
Event		*event;
{
    Card	card = setup_get(ws, WS_CARD_NAME, panel_get_value(item));
            
    if (!card)
        add_card(item, event);
    else
        toggle_card(card);
}


static void
close_client(item, event)
Panel_item	item;
Event		*event;
{

    client_selected(edit_client_item, event);

}

static void
close_card(item, event)
Panel_item	item;
Event		*event;
{

    card_selected(edit_card_item, event);

}


static void
toggle_client(client)
Client		client;
{
    Panel_item	item = setup_get(client, SETUP_OPAQUE, SETUP_ALL);
    Object_info	*info;
    Client	old_edit_client;
    int		x, i;
    int		delay = 500;
    int		step = 8;
    
    panel_set(item, PANEL_SHOW_ITEM, FALSE, 0);
    if (item == edit_client_item) {
	/* close client open for edit */

	panel_set(item, PANEL_LABEL_STRING, "", 0);
	glue(item, 0);

        show_client_info(0);

	glue(client_item, client);
	panel_set(client_item, PANEL_SHOW_ITEM, TRUE, 0);

        client_item = 0;

	return;
    }

    /* open client for edit */

    /* remember the client that used to
     * own the edit_client_item.
     */
    info = (Object_info *) panel_get(edit_client_item, PANEL_CLIENT_DATA);
    old_edit_client = (Client) info->obj;

    /* prepare the open edit client to
     * look like the newly selected client.
     */
    panel_set(edit_client_item,           
	      PANEL_LABEL_IMAGE, panel_get(item, PANEL_LABEL_IMAGE),
	      PANEL_PAINT, PANEL_NONE,
	      0);
    glue(edit_client_item, client);
	   
    if (client_item) {
	/* place the new open edit client to the
	 * left, and prepare the temporary dance client
	 * to dance on the right.
	 */
	panel_set(edit_client_item, PANEL_ITEM_X, 100, 0);
	panel_set(dance_client, 
		  PANEL_ITEM_X, OPEN_X,
		  PANEL_LABEL_IMAGE, 
		      panel_get(client_item, PANEL_LABEL_IMAGE),
		  PANEL_SHOW_ITEM, TRUE, 
		  0);

	/* lock-step dance */
	for (x = 100; x < OPEN_X; x += step) {
	    panel_set(dance_client, PANEL_ITEM_X, x + 64, 0);
	    panel_set(edit_client_item, PANEL_ITEM_X, x, 0);
	    for (i = 1; i < delay; i++);
	}   
	panel_set(dance_client, PANEL_SHOW_ITEM, FALSE, 0);

	/* open client has danced back up to the
	 * client panel.
	 */
	glue(client_item, old_edit_client);
	panel_set(client_item, PANEL_SHOW_ITEM, TRUE, 0);
    } else
	/* no previous item open, so no dance */
	panel_set(edit_client_item,
		  PANEL_ITEM_X, OPEN_X,
		  PANEL_SHOW_ITEM, TRUE, 
		  0);

    /* show the client fields */
    show_client_info(client);

    /* remember the item in the client panel
     * corresponding to the edit_client_item.
     */
    client_item = item;
}


static void
show_client_info(client)
register	Client	client;
{
    
    register Panel_item		item;
    register int		row;
    register short		show_it = client != 0;
    
    FOREACH_ROW(row)
	item = client_items[row];
	glue(item, client);
	panel_set(item, PANEL_SHOW_ITEM, show_it, 0);
    END_FOREACH
    
    if (show_it) {
        /* make sure the edit text item has
         * the current client name.
         */
        panel_set_value(client_edit, setup_get(client, CLIENT_NAME));
        set_caret(client_edit, 0);
    } else
        panel_set_value(client_edit, "");

    panel_set(client_close, PANEL_SHOW_ITEM, show_it, 0);
    panel_set(client_delete, PANEL_SHOW_ITEM, show_it, 0);
}


static void
toggle_card(card)
Card		card;
{
    Panel_item  item = setup_get(card, SETUP_OPAQUE, SETUP_ALL);
    Object_info *info;
    Card	old_edit_card;
    int		x, i;
    int		delay = 1000;
    int		step = 8;

    panel_set(item, PANEL_SHOW_ITEM, FALSE, 0);
    if (item == edit_card_item) {
        /* close card open for edit */
        
        panel_set(item, PANEL_LABEL_STRING, "", 0);
        glue(item, 0);
 
        show_card_info(0);
 
        glue(card_item, card);
        panel_set(card_item, PANEL_SHOW_ITEM, TRUE, 0);
 
        card_item = 0;
 
        return;
    } 
    
    /* open card for edit */
 
    /* remember the card that used to
     * own the edit_card_item.
     */
    info = (Object_info *) panel_get(edit_card_item, PANEL_CLIENT_DATA);
    old_edit_card = (Card) info->obj;
 
    /* prepare the open edit card to
     * look like the newly selected card.
     */
    panel_set(edit_card_item,
              PANEL_LABEL_IMAGE, panel_get(item, PANEL_LABEL_IMAGE),
              PANEL_PAINT, PANEL_NONE,
              0);
    glue(edit_card_item, card);
        
    if (card_item) {
        /* place the new open edit card to the
         * left, and prepare the temporary dance card
         * to dance on the right.
         */
        panel_set(edit_card_item, PANEL_ITEM_X, 100, 0);
        panel_set(dance_card,
                  PANEL_ITEM_X, OPEN_X,
                  PANEL_LABEL_IMAGE,
                      panel_get(card_item, PANEL_LABEL_IMAGE),
                  PANEL_SHOW_ITEM, TRUE,
                  0);
 
        /* lock-step dance */
        for (x = 100; x < OPEN_X; x += step) {
            panel_set(dance_card, PANEL_ITEM_X, x + 64, 0);
            panel_set(edit_card_item, PANEL_ITEM_X, x, 0);
            for (i = 1; i < delay; i++);
        }
        panel_set(dance_card, PANEL_SHOW_ITEM, FALSE, 0);
 
        /* open card has danced back up to the
         * card panel.
         */
        glue(card_item, old_edit_card);
        panel_set(card_item, PANEL_SHOW_ITEM, TRUE, 0);
    } else
        /* no previous item open, so no dance */
        panel_set(edit_card_item,
                  PANEL_ITEM_X, OPEN_X,
                  PANEL_SHOW_ITEM, TRUE,
                  0);
 
    /* show the card fields */
    show_card_info(card);
 
    /* remember the item in the card panel
     * corresponding to the edit_card_item.
     */
    card_item = item;        
}  


static void
show_card_info(card)
register	Card	card;
{
    
    register Panel_item		item;
    register int		row;
    register short		show_it = card != 0;
    
    FOREACH_ROW(row)
	item = card_items[row];
	if (!item)
	    continue;

	glue(item, card);
	panel_set(item, PANEL_SHOW_ITEM, show_it, 0);
    END_FOREACH
    
    if (show_it) {
        /* make sure the edit text item has
         * the current card name.
         */
        panel_set_value(card_edit, setup_get(card, CLIENT_NAME));
 
        set_caret(card_edit, 0);
        panel_set(card_clear_default, PANEL_SHOW_ITEM, FALSE, 0);
        panel_set(card_make_default, PANEL_SHOW_ITEM, TRUE, 0);
    } else { 
        panel_set_value(card_edit, "");
        panel_set(card_make_default, PANEL_SHOW_ITEM, FALSE, 0);
        panel_set(card_clear_default, PANEL_SHOW_ITEM, TRUE, 0);
    }  

    panel_set(card_delete, PANEL_SHOW_ITEM, show_it, 0);
    panel_set(card_close, PANEL_SHOW_ITEM, show_it, 0);

    glue(card_apply_to, card);
    panel_set(card_apply_to, PANEL_SHOW_ITEM, show_it, 0);
}


static void
get_nd_partitions()
{

    get_object_choices(ws, WS_ND_PARTITION, HARD_NAME, 
	client_items[ord(ROW_ROOT_DISK)]);
    panel_set(client_items[ord(ROW_ROOT_DISK)],
              CYCLE_ATTRS(INDENT, ATTR_ROW(ord(ROW_ROOT_DISK) + 4), 
              		  VALUE_OFFSET - INDENT),
              PANEL_PAINT, PANEL_NONE,
    	      0);

    get_object_choices(ws, WS_ND_PARTITION, HARD_NAME, 
	client_items[ord(ROW_SWAP_DISK)]);
    panel_set(client_items[ord(ROW_SWAP_DISK)],
              CYCLE_ATTRS(INDENT, ATTR_ROW(ord(ROW_SWAP_DISK) + 4), 
              		  VALUE_OFFSET - INDENT),
              PANEL_PAINT, PANEL_NONE,
    	      0);

    get_choices(ws, CONFIG_CARD_ND, card_items[ord(ROW_ROOT_DISK)]);
    panel_set(card_items[ord(ROW_ROOT_DISK)],
              CYCLE_ATTRS(INDENT, ATTR_ROW(ord(ROW_ROOT_DISK) + 4), 
              		  VALUE_OFFSET - INDENT),
              PANEL_PAINT, PANEL_NONE,
    	      0);

    get_choices(ws, CONFIG_CARD_ND, card_items[ord(ROW_SWAP_DISK)]);
    panel_set(card_items[ord(ROW_SWAP_DISK)],
              CYCLE_ATTRS(INDENT, ATTR_ROW(ord(ROW_SWAP_DISK) + 4), 
              		  VALUE_OFFSET - INDENT),
              PANEL_PAINT, PANEL_NONE,
    	      0);
}


static Panel_setting
set_client_name(item, event)
Panel_item	item;
Event		*event;
{
    Object_info *info   = (Object_info *) panel_get(item, PANEL_CLIENT_DATA);
    char        *name	= (char *) panel_get_value(item);

    setup_set(info->obj, CLIENT_NAME, name, 0);
    image_relabel_pr(panel_get(edit_client_item, PANEL_LABEL_IMAGE),
                     setup_get(info->obj, CLIENT_NAME), -1);
    panel_set(edit_client_item, PANEL_PAINT, PANEL_NO_CLEAR, 0);
    return panel_text_notify(item, event);
}


static Panel_setting
set_card_name(item, event)
Panel_item	item;
Event		*event;
{
    Object_info *info   = (Object_info *) panel_get(item, PANEL_CLIENT_DATA);
    char        *name  = (char *) panel_get_value(item);

    setup_set(info->obj, CLIENT_NAME, name, 0);
    image_relabel_pr(panel_get(edit_card_item, PANEL_LABEL_IMAGE),
                     setup_get(info->obj, CLIENT_NAME), -1);
    panel_set(edit_card_item, PANEL_PAINT, PANEL_NO_CLEAR, 0);
    return panel_text_notify(item, event);
}


static void
add_client(item, event)
Panel_item	item;
Event		*event;
{
    Client	client;
    int		x, y;

    client = setup_create(CLIENT, CLIENT_NAME, panel_get_value(item),
			          SETUP_CALLBACK, handle_error,
			          0);
    if(!client)
        return;			          
    assign_client_position(&x, &y);
    item = panel_create_item(client_panel, PANEL_BUTTON,
		      PANEL_LABEL_X, x,
		      PANEL_LABEL_Y, y,
		      PANEL_LABEL_IMAGE, 
			  image_label_pr(image_client,
					 setup_get(client, CLIENT_NAME),
					 -1),
		      PANEL_CLIENT_DATA, 
			  object_info(client, SETUP_ALL),
		      PANEL_SHOW_ITEM, FALSE, 
		      PANEL_NOTIFY_PROC, client_selected,	
		      0 );
    glue(item, client);

    setup_set(ws, WS_CLIENT, SETUP_APPEND, client, 0);

    toggle_client(client);
}


static void
delete_client(item, event)
Panel_item	item;
Event		*event;
{
    Object_info	*info;
    Client	client;

    if (!client_item)
       return;

    info = (Object_info *) panel_get(edit_client_item, PANEL_CLIENT_DATA);
    client = (Client) info->obj;

    panel_set_value(client_edit, "");
    show_client_info(0);
    panel_set(edit_client_item, 
	      PANEL_LABEL_STRING, "", 
	      PANEL_SHOW_ITEM, FALSE, 
	      0);
    glue(edit_client_item, 0);

    prs_destroy((Pixrect *) panel_get(client_item, PANEL_LABEL_IMAGE));
    info = (Object_info *) panel_get(client_item, PANEL_CLIENT_DATA);
    free(info);
    panel_free(client_item);
    client_item = 0;

    setup_destroy(ws, WS_CLIENT_NAME, setup_get(client, CLIENT_NAME), 0);
}


static void
add_card(item, event)
Panel_item	item;
Event		*event;
{
    Card	card;
    int		x, y;

    card = setup_create(CARD, 
			CLIENT_NAME, panel_get_value(item),
			SETUP_CALLBACK, handle_error,
			0);
    if(!card)
        return;			          

    assign_card_position(&x, &y);
    item = panel_create_item(card_panel, PANEL_BUTTON,
		      PANEL_LABEL_X, x,
		      PANEL_LABEL_Y, y,
		      PANEL_LABEL_IMAGE, 
			  image_label_pr(image_card,
					 setup_get(card, CLIENT_NAME),
					 -1),
		      PANEL_CLIENT_DATA, 
			  object_info(card, SETUP_ALL),
		      PANEL_NOTIFY_PROC, card_selected,	
		      0 );
    glue(item, card);

    setup_set(ws, WS_CARD, SETUP_APPEND, card, 0);

    toggle_card(card);
}


static void
delete_card(item, event)
Panel_item	item;
Event		*event;
{
    Object_info	*info;
    Card	card;

    if (!card_item)
       return;

    info = (Object_info *) panel_get(edit_card_item, PANEL_CLIENT_DATA);
    card = (Card) info->obj;

    show_card_info(0);
    panel_set(edit_card_item, 
	      PANEL_LABEL_STRING, "", 
	      PANEL_SHOW_ITEM, FALSE, 
	      0);
    glue(edit_card_item, 0);

    prs_destroy((Pixrect *) panel_get(card_item, PANEL_LABEL_IMAGE));
    info = (Object_info *) panel_get(card_item, PANEL_CLIENT_DATA);
    free(info);
    panel_free(card_item);
    card_item = 0;

    setup_destroy(ws, WS_CARD_NAME, setup_get(card, CLIENT_NAME), 0);
}


static void
assign_client_position(x, y)
int	*x, *y;
{
    static int	next_x 		= 15;
    static int	next_y 		= 20;

    *x = next_x;
    *y = next_y;
    next_x += TAB;
    if (next_x > client_panel_rect.r_width - image_client->pr_width - 15) {
	next_y += 80;
	next_x = 15;
    }
}


static void
assign_card_position(x, y)
int	*x, *y;
{
    static int	next_x 		= 15;
    static int	next_y 		= 20;

    *x = next_x;
    *y = next_y;
    next_x += TAB;
    if (next_x > card_panel_rect.r_width - image_card->pr_width - 15) {
	next_y += 80;
	next_x = 15;
    }
}

static void
client_selected(item, event)
Panel_item      item;
Event           *event;
{
    Object_info *info    = (Object_info *) panel_get(item, PANEL_CLIENT_DATA);

    toggle_client((Client) info->obj);
}


static void
card_selected(item, event)
Panel_item      item;
Event           *event;
{
    Object_info *obj    = (Object_info *) panel_get(item, PANEL_CLIENT_DATA);

    toggle_card((Card) obj->obj);
}


/* Make the card currently open for edit the default
 * If none is open for edit, clear the default
 */
static void
make_default_card(item, event)
Panel_item      item;
Event           *event;
{
    char        *card_name  = panel_get_value(card_edit);
    Card	card;
 
 
    if (strcmp(card_name, "") == 0) {		/* clear default card */
        setup_set(ws, WS_DEFAULT_CARD, 0, 0);
        					/* clear default card name */
        panel_set_value(default_card_name, "");
        					/* hide the card image */
        panel_set(default_card_image, PANEL_SHOW_ITEM, FALSE, 0);
    }    
    else if (card = (Card) setup_get(ws, WS_CARD_NAME, card_name)) {
                  				/* set a default card */
            setup_set(ws, WS_DEFAULT_CARD, card, 0);
        					/* update default card name */
            panel_set_value(default_card_name, card_name);
                  				/* show the card image */
            image_relabel_pr(panel_get(default_card_image, PANEL_LABEL_IMAGE),
                             card_name, -1);
            panel_set(default_card_image, PANEL_SHOW_ITEM, TRUE, 0);
    }    
}


/* Make the card specified by name the default
 * If name is "", clear the default
 */
static Panel_setting
set_default_card(item, event)
Panel_item      item;
Event           *event;
{
    char        *card_name  = (char *) panel_get_value(item);
    Card	card;
 
    if (strcmp(card_name, "") == 0) {		/* clear default card */
        setup_set(ws, WS_DEFAULT_CARD, 0, 0);
        					/* hide the card image */
        panel_set(default_card_image, PANEL_SHOW_ITEM, FALSE, 0);
    } else {
        if (card = (Card) setup_get(ws, WS_CARD_NAME_TO_DEFAULT, card_name)) {
                  				/* set a default card */
            setup_set(ws, WS_DEFAULT_CARD, card, 0);
                  				/* show the card image */
            image_relabel_pr(panel_get(default_card_image, PANEL_LABEL_IMAGE),
                             card_name, -1);
            panel_set(default_card_image, PANEL_SHOW_ITEM, TRUE, 0);
	} else {
            setup_set(ws, WS_DEFAULT_CARD, 0, 0);
        					/* clear default card name */
            panel_set_value(default_card_name, "");
        					/* hide the card image */
            panel_set(default_card_image, PANEL_SHOW_ITEM, FALSE, 0);
	    return PANEL_NONE;
	}
    }    
    return set_caret(item, event);
}
 

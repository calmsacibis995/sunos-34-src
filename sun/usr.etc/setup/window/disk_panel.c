#ifndef lint
static	char sccsid[] = "@(#)disk_panel.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup.h"

static void		init_disk_panel();

static void		init_info_area();
static void		show_info_area();

static void		init_slots();
static void		convert_heading_info();

static void		edit_disk_choice();
static void		edit_disk_button();
static void		toggle_disk();
static void		show_hp_row();
 
static int		partition_scale();

static void		init_edit_area();
static void		show_edit_area();

static void		edit_hp_button();
static void		edit_hp_choice();
static void		adjust_hp_size();
static void		set_hp_size();
static void		move_hp();
static void		get_hp_choices();
static void		get_move_to_choices();


#define	MAX_SLOTS	1	/* only space for one open disk */

#define	SLOT_X_GAP	30	/* space between slot columns */
#define	SLOT_Y_GAP	80	/* space between slot rows */

#define	HP_X_GAP	30	/* horizontal space between data columns */
#define	HP_Y_GAP	7	/* vertical space between hard partitions */

static int	hp_y_height;	/* height of each hard partition row */

static int	slot_width;
static int	slot_height;

typedef enum {
    INFO_CLOSE,
    INFO_SIZE_LABEL,
    INFO_SIZE,
    INFO_FREE_LABEL,
    INFO_FREE,
    INFO_OVERLAP,
    INFO_ROUND,
    INFO_FLOAT,
    INFO_HOG,
    INFO_LAST
} Disk_info;


typedef enum {
    HDG_LETTER,
    HDG_TYPE,
    HDG_SIZE,
    HDG_PARTITION,
    HDG_LAST
} Heading_name;


typedef enum {
    EDIT_CLOSE,
    EDIT_OFFSET,
    EDIT_SIZE,
    EDIT_SIZE_SLIDER,
    EDIT_TYPE,
    EDIT_MOUNT,
    EDIT_MOVE_LABEL,
    EDIT_MOVE_WHAT,
    EDIT_MOVE_TO,
    EDIT_MOVE_DO,
    EDIT_LAST
} Edit_info;


typedef struct  {
    char		*title;
    Setup_attribute	attr;
    short	 	data_offset, length;
} Heading;

static Heading	heading_info[] = {
    "LETTER",		HARD_LETTER, 		3, 6,
    "HARD PARTITION",	HARD_WHAT_IT_IS, 	0, 15,
    "SIZE (K Bytes)",	HARD_SIZE_STRING, 	0, 20,
    "PARTITION MAP",	SETUP_NONE, 		0, 80
};

typedef Panel_item	*Hp_row;

#define	disk_info(which)		disk_info_items[ord(which)]
#define	edit_info(which)		edit_info_items[ord(which)]


#define	FOREACH_COLUMN(col)	\
    { for (col = 0; col < ord(HDG_LAST); col++) {


#define	FOREACH_INFO_ITEM(item)	\
    { register int	_i; \
      for (_i = 0; _i < ord(INFO_LAST); _i++) { \
	  item = disk_info_items[_i];


#define	FOREACH_EDIT_ITEM(item)	\
    { register int	_i; \
      for (_i = 0; _i < ord(EDIT_LAST); _i++) { \
	  item = edit_info_items[_i];


#define END_FOREACH	}}

typedef struct {
    Panel_item	closed_disk;
    Panel_item	open_disk;
    Panel_item	headings[ord(HDG_LAST)];
    int		data_x, data_y;
    int		map_x;
} Slot_info;


static Panel		panel = 0;

static Slot_info	slot_info[MAX_SLOTS];

static Panel_item	disk_info_items[ord(INFO_LAST)];
static Panel_item	edit_info_items[ord(EDIT_LAST)];

static Panel_item	edit_disk_item, display_units, edit_hp_item;

static Disk		edit_disk;
static Hard_partition	edit_hp;


static Hp_row		init_hp_row();
static Slot_info	*find_free_slot();



/* Show the disks screen */
void
disk_show(disk, show_it)
Disk	disk;
int	show_it;
{
    if (!panel)
       init_disk_panel();

    show_panel(panel, show_it);

    if (disk) {
	Panel_item item = (Panel_item) setup_get(disk, SETUP_OPAQUE, SETUP_ALL);
	Object_info *info = (Object_info *) panel_get(item, PANEL_CLIENT_DATA);
	
	/* open the disk if not already open */
	if (!info->other) {
	    /* make sure we hide the disk panel
	     * before we toggle the disk.
	     */
	    if (show_it)
		show_panel(panel, FALSE);
	    toggle_disk(disk);
	    if (show_it)
		show_panel(panel, TRUE);
	}
    }
}


/* Initialize the disks panel */
static void
init_disk_panel()
{
    Panel_item		disk_item;
    Controller		cont;
    Disk		disk;
    Hard_partition	hp;
    Pixrect		*disk_image;
    int			tab = 70;
    register int	cont_index, disk_index, hp_index;
    int 		x, y;
    
    panel = make_panel(display_rect);
    
    /* edit disk item */
    edit_disk_item = 
	panel_create_item(panel, PANEL_CHOICE,
	    CYCLE_ATTRS(25, 80, 130),
	    PANEL_CHOICE_STRINGS, 	"None", 0,	/* first choice */
	    PANEL_LABEL_IMAGE, 		image_string("Edit Disk:"),
	    PANEL_MENU_TITLE_STRING, 	"Edit Disk:",
	    PANEL_NOTIFY_PROC, 		edit_disk_choice,
	    0);

    /* Disk icons */
    x = 50;
    y = 4;
    SETUP_FOREACH_OBJECT(ws, WS_CONTROLLER, cont_index, cont)
        SETUP_FOREACH_OBJECT(cont, CONTROLLER_DISK, disk_index, disk)
            disk_item = setup_get(disk, SETUP_OPAQUE, SETUP_ALL);
	    disk_image = (Pixrect *) panel_get(disk_item, PANEL_LABEL_IMAGE);
            disk_item = panel_create_item(panel, PANEL_BUTTON,
			      PANEL_ITEM_X, x,
    		              PANEL_ITEM_Y, y,
    			      PANEL_LABEL_IMAGE, disk_image,
   		              PANEL_CLIENT_DATA, 
				  object_info_plus(disk, SETUP_ALL, 0),
			      PANEL_NOTIFY_PROC, edit_disk_button,
			      0);
	    glue(disk_item, disk);

	    /* add the disk name to the edit disk item */
	    panel_set(edit_disk_item, 
		PANEL_CHOICE_STRING, 
		    setup_get(ws, WS_DISK_INDEX, disk) + 1, 
		    setup_get(disk, DISK_NAME),
		PANEL_PAINT, PANEL_NONE,
		0);

	    x += tab;
	    /* set the callback proc for each
	     * hard partition.
	     */
	    SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, hp_index, hp)
		setup_set(hp, SETUP_CALLBACK, handle_error, 0);
	    SETUP_END_FOREACH
    	SETUP_END_FOREACH
    SETUP_END_FOREACH

    /* initialize the edit disk info area */
    init_info_area();

    /* initialize the open disk slots */
    init_slots(panel);

    /* initialize the hard partition edit items */
    init_edit_area(panel);
}


static void
init_info_area()
{
    register int	x, y, value_offset;

    x = 200;
    y = 80;

    disk_info(INFO_CLOSE) =
	panel_create_item(panel, PANEL_BUTTON,
	    PANEL_LABEL_X, x,
	    PANEL_LABEL_Y, y,
	    PANEL_LABEL_IMAGE, image_close,
	    PANEL_NOTIFY_PROC, edit_disk_button,
	    PANEL_SHOW_ITEM, FALSE,
	    0);

    x = 25;
    y = 110;

    disk_info(INFO_SIZE_LABEL) =
	panel_create_item(panel, PANEL_MESSAGE,
	    PANEL_LABEL_X, x,
	    PANEL_LABEL_Y, y,
	    PANEL_LABEL_STRING, "Total Size:",
	    PANEL_SHOW_ITEM, FALSE,
	    0);

    x = 125;
    disk_info(INFO_SIZE) =
	panel_create_item(panel, PANEL_MESSAGE,
	    PANEL_LABEL_X, x,
	    PANEL_LABEL_Y, y,
	    PANEL_CLIENT_DATA, object_info(0, DISK_SIZE_STRING_LEFT),
	    PANEL_SHOW_ITEM, FALSE,
	    0);


    x = 200;
    disk_info(INFO_FREE_LABEL) =
	panel_create_item(panel, PANEL_MESSAGE,
	    PANEL_LABEL_X, x,
	    PANEL_LABEL_Y, y,
	    PANEL_LABEL_STRING, "Free:",
	    PANEL_SHOW_ITEM, FALSE,
	    0);

    x = 270;
    disk_info(INFO_FREE) =
	panel_create_item(panel, PANEL_MESSAGE,
	    PANEL_LABEL_X, x,
	    PANEL_LABEL_Y, y,
	    PANEL_CLIENT_DATA, object_info(0, DISK_FREE_SPACE_STRING_LEFT),
	    PANEL_SHOW_ITEM, FALSE,
	    0);


    x = display_rect.r_width - (display_rect.r_width/3);
    value_offset = 190;
    y = 65;

    display_units = 
	panel_create_item(panel, PANEL_CHOICE,
	    PANEL_LABEL_IMAGE,		image_string("Display Units:"),
	    PANEL_MENU_TITLE_STRING,	"Display Units:",
	    PANEL_NOTIFY_PROC, 		set_info,
	    PANEL_SHOW_ITEM,		FALSE,
	    PANEL_CLIENT_DATA, object_info(ws, PARAM_DISK_DISPLAY_UNITS),
	    0);
    get_choices(ws, CONFIG_DISK_DISPLAY_UNITS, display_units);
    panel_set(display_units, CYCLE_ATTRS(x, y, value_offset), 0);

    y += 35;

    disk_info(INFO_ROUND) =
	panel_create_item(panel, PANEL_CHOICE,
	    CYCLE_ATTRS(x, y, value_offset),
	    PANEL_LABEL_IMAGE, 	image_string("Round to Cylinders:"),
	    PANEL_MENU_TITLE_STRING,	"Round to Cylinders:",
	    PANEL_CHOICE_STRINGS, 	"No", "Yes", 0,
	    PANEL_NOTIFY_PROC, 		set_info,
	    PANEL_SHOW_ITEM, 		FALSE,
	    PANEL_CLIENT_DATA, object_info(0, DISK_PARAM_CYLROUNDING),
	    0);

    y += 20;

    disk_info(INFO_OVERLAP) =
	panel_create_item(panel, PANEL_CHOICE,
	    CYCLE_ATTRS(x, y, value_offset),
	    PANEL_LABEL_IMAGE, 	image_string("Overlapping Allowed:"),
	    PANEL_MENU_TITLE_STRING,	"Overlapping Allowed:",
	    PANEL_CHOICE_STRINGS, 	"No", "Yes", 0,
	    PANEL_NOTIFY_PROC, 		set_info,
	    PANEL_SHOW_ITEM, 		FALSE,
	    PANEL_CLIENT_DATA, object_info(0, DISK_PARAM_OVERLAPPING),
	    0);

    y += 20;

    disk_info(INFO_FLOAT) =
	panel_create_item(panel, PANEL_CHOICE,
	    CYCLE_ATTRS(x, y, value_offset),
	    PANEL_LABEL_IMAGE, 	image_string("Float:"),
	    PANEL_MENU_TITLE_STRING,	"Float:",
	    PANEL_CHOICE_STRINGS, 	"No", "Yes", 0,
	    PANEL_NOTIFY_PROC, 		set_info,
	    PANEL_SHOW_ITEM, 		FALSE,
	    PANEL_CLIENT_DATA, object_info(0, DISK_PARAM_FLOATING),
	    0);

    y += 20;

    disk_info(INFO_HOG) =
	panel_create_item(panel, PANEL_CHOICE,
	    CYCLE_ATTRS(x, y, value_offset),
	    PANEL_LABEL_IMAGE, 		image_string("Free Space Hog:"),
	    PANEL_MENU_TITLE_STRING,	"Free Space Hog:",
	    PANEL_CHOICE_STRINGS, 	"None", 0,	/* first choice */
	    PANEL_NOTIFY_PROC, 		set_info,
	    PANEL_SHOW_ITEM, 		FALSE,
	    PANEL_CLIENT_DATA, object_info(0, DISK_FREE_HOG_INDEX),
	    0);
}



/* Initialize the open disk slots.
 * Compute the position of each slot, and
 * create the necessary panel items.
 */
static void
init_slots(panel)
Panel	panel;
{
    Pixfont	*font		= (Pixfont *) panel_get(panel, PANEL_FONT);
    int		char_width	= font->pf_defaultsize.x;
    int		char_height	= font->pf_defaultsize.y;
    
    int		first_slot_x	= 10;
    int		first_slot_y	= display_rect.r_height / 4 + 30;

    int		last_slot_x;

    int		disk_x_offset;
    int		disk_y_offset;

    register int	index;
    register Slot_info	*slot;
    register int	x	= first_slot_x;
    register int	y	= first_slot_y;
    register int	col;

    char		*title;
    int			length;
    
   
    /* convert the heading char units
     * to pixel units.
     */
    convert_heading_info(char_width);
    
    slot_width	= display_rect.r_width;
    slot_height	= 250;
    hp_y_height	= char_height;
    
    last_slot_x		= display_rect.r_width - slot_width;

    disk_x_offset	= 130;
    disk_y_offset	= image_disk->pr_height + 10;

    for (index = 0; index < MAX_SLOTS; index++) {
        slot		= &slot_info[index];
         
        /* record the upper left point of
         * the first data column.
         */       
        slot->data_x	= x;
        slot->data_y	= y + hp_y_height + HP_Y_GAP;
        
        slot->closed_disk = (Panel_item) 0;
        slot->open_disk = 
            panel_create_item(panel, PANEL_BUTTON,
			      PANEL_ITEM_X, x + disk_x_offset,
    		              PANEL_ITEM_Y, y - disk_y_offset,
			      PANEL_CLIENT_DATA, 
				  object_info_plus(0, SETUP_ALL, slot),
			      PANEL_NOTIFY_PROC, edit_disk_button,
			      PANEL_SHOW_ITEM, FALSE,
			      0);
	FOREACH_COLUMN(col)
            title	= heading_info[col].title;
            length	= heading_info[col].length;
            slot->headings[col] =
                panel_create_item(panel, PANEL_MESSAGE,
                    PANEL_ITEM_X, x + (length - char_width * strlen(title)) / 2,
                    PANEL_ITEM_Y, y,
                    PANEL_LABEL_STRING, title,
		    PANEL_SHOW_ITEM, FALSE,
                    0);
	    /* update the map position */
	    slot->map_x = x;
            x += length + HP_X_GAP;
	END_FOREACH
	x += SLOT_X_GAP;
        if (x >= last_slot_x) {
            x = first_slot_x;
            y += slot_height + SLOT_Y_GAP;
        }
    }
}


/* convert the character unit info
 * in global haeding_info[] to pixel units.
 */
static void
convert_heading_info(char_width)
register int	char_width;
{
    register int	col;
    
    FOREACH_COLUMN(col)
        heading_info[col].length	*= char_width;
        heading_info[col].data_offset	*= char_width;
    END_FOREACH
};

    
static Hp_row
init_hp_row()
{
    register Hp_row	hp_row;
    register int	col;
    Panel_item		item;

    hp_row = (Hp_row) calloc(ord(HDG_LAST), sizeof(Panel_item));
    FOREACH_COLUMN(col)
	item = 
	    panel_create_item(panel, PANEL_BUTTON, 
	        PANEL_CLIENT_DATA, object_info(0, heading_info[col].attr),
		PANEL_NOTIFY_PROC, edit_hp_button,
	        PANEL_SHOW_ITEM, FALSE,
	        0);
	hp_row[col] = item;
    END_FOREACH
    return hp_row;
}


static void
init_edit_area(panel)
Panel	panel;
{

    Pixfont             *font = (Pixfont *) panel_get(panel, PANEL_FONT);
    int	            	char_height = font->pf_defaultsize.y;
    register int	x = 410;
    register int        y=display_rect.r_height/4+65+((char_height+HP_Y_GAP)*8);
    register int	value_offset = 180;
    register int 	value_x = x + value_offset;

    edit_hp_item =
	panel_create_item(panel, PANEL_CHOICE,
	    CYCLE_ATTRS(x - 20, y, value_offset + 20),
	    PANEL_CHOICE_STRINGS, 	"None", 0,	/* first choice */
	    PANEL_LABEL_IMAGE, 		image_string("Edit Hard Partition:"),
	    PANEL_MENU_TITLE_STRING, 	"Edit Hard Partition:",
	    PANEL_NOTIFY_PROC, 		edit_hp_choice,
	    PANEL_SHOW_ITEM, 		FALSE,
	    0);

    edit_info(EDIT_CLOSE) =
	panel_create_item(panel, PANEL_BUTTON,
	    PANEL_LABEL_X, value_x + 240,
	    PANEL_LABEL_Y, y,
	    PANEL_LABEL_IMAGE, image_close,
	    PANEL_NOTIFY_PROC, edit_hp_button,
	    PANEL_SHOW_ITEM, FALSE,
	    0);

    y += 30;

    edit_info(EDIT_OFFSET) =
	panel_create_item(panel, PANEL_TEXT,
	    PANEL_LABEL_X, x,
	    PANEL_LABEL_Y, y,
	    PANEL_VALUE_Y, y + 4,
	    PANEL_VALUE_X, value_x,
	    PANEL_LABEL_IMAGE, image_string("Offset:"),
	    PANEL_MENU_TITLE_STRING, "Offset:",
	    PANEL_VALUE_DISPLAY_LENGTH, 15,
	    PANEL_VALUE_STORED_LENGTH, 80,
	    PANEL_CLIENT_DATA, object_info(0, HARD_OFFSET_STRING_LEFT),
	    PANEL_NOTIFY_PROC, set_text_info,
	    PANEL_SHOW_ITEM, FALSE,
	    0);
	    
    y += 30;

    edit_info(EDIT_SIZE) =
	panel_create_item(panel, PANEL_TEXT,
	    PANEL_LABEL_X, x,
	    PANEL_LABEL_Y, y,
	    PANEL_VALUE_Y, y + 4,
	    PANEL_VALUE_X, value_x,
	    PANEL_LABEL_IMAGE, image_string("Size:"),
	    PANEL_MENU_TITLE_STRING, "Size:",
	    PANEL_VALUE_DISPLAY_LENGTH, 15,
	    PANEL_VALUE_STORED_LENGTH, 80,
	    PANEL_CLIENT_DATA, object_info(0, HARD_SIZE_STRING_LEFT),
	    PANEL_NOTIFY_PROC, set_text_info,
	    PANEL_SHOW_ITEM, FALSE,
	    0);
	    
    edit_info(EDIT_SIZE_SLIDER) =
	panel_create_item(panel, PANEL_SLIDER,
	    PANEL_LABEL_X, value_x + 130,
	    PANEL_LABEL_Y, y,
	    PANEL_SLIDER_WIDTH, 200,
	    PANEL_SHOW_VALUE, FALSE,
	    PANEL_EVENT_PROC, set_hp_size,
	    PANEL_NOTIFY_LEVEL, PANEL_ALL,
	    PANEL_NOTIFY_PROC, adjust_hp_size,
	    PANEL_SHOW_ITEM, FALSE,
	    0);

    y += 30;

    edit_info(EDIT_TYPE) =
	panel_create_item(panel, PANEL_CHOICE,
	    PANEL_LABEL_IMAGE, 		image_string("Type:"),
	    PANEL_MENU_TITLE_STRING, 	"Type:",
	    PANEL_NOTIFY_PROC, 		set_info,
	    PANEL_SHOW_ITEM, 		FALSE,
	    PANEL_CLIENT_DATA, 		object_info(0, HARD_TYPE),
	    0);
    get_choices(ws, CONFIG_HARD_PARTITION_TYPE, edit_info(EDIT_TYPE));
    panel_set(edit_info(EDIT_TYPE), 
	    CYCLE_ATTRS(x, y, value_offset),
	    PANEL_PAINT, PANEL_NONE,
	    0);
	    
    y += 30;

    edit_info(EDIT_MOUNT) =
	panel_create_item(panel, PANEL_TEXT,
	    PANEL_LABEL_X, 		x + 2,
	    PANEL_LABEL_Y, 		y,
	    PANEL_VALUE_X, 		value_x,
	    PANEL_VALUE_Y, 		y + 4,
	    PANEL_LABEL_IMAGE, 		image_string("Mount Point:"),
	    PANEL_MENU_TITLE_STRING, 	"Mount Point:",
	    PANEL_VALUE_DISPLAY_LENGTH, 60,
	    PANEL_VALUE_STORED_LENGTH, 	255,
	    PANEL_CLIENT_DATA, 		object_info(0, HARD_MOUNT_PT),
	    PANEL_NOTIFY_PROC, 		set_text_info,
	    PANEL_SHOW_ITEM, 		FALSE,
	    0);

    y += 30;

    edit_info(EDIT_MOVE_LABEL) =
	panel_create_item(panel, PANEL_MESSAGE,
	    PANEL_LABEL_X, x + 5,
	    PANEL_LABEL_Y, y + 4,
	    PANEL_LABEL_STRING, "Move:",
	    PANEL_SHOW_ITEM, FALSE,
	    0);

    edit_info(EDIT_MOVE_WHAT) =
	panel_create_item(panel, PANEL_MESSAGE,
	    PANEL_LABEL_X, value_x,
	    PANEL_LABEL_Y, y + 4,
	    PANEL_SHOW_ITEM, FALSE,
	    0);

    edit_info(EDIT_MOVE_TO) =
	panel_create_item(panel, PANEL_CHOICE,
	    CYCLE_ATTRS(value_x + 130, y, 60),
	    PANEL_CHOICE_STRINGS, "xy0a", 0,	/* dummy choice */
	    PANEL_LABEL_IMAGE, image_string("To:"),
	    PANEL_MENU_TITLE_STRING, "To:",
	    PANEL_SHOW_ITEM, FALSE,
	    0);
    get_move_to_choices(edit_info(EDIT_MOVE_TO));

    edit_info(EDIT_MOVE_DO) =
	panel_create_item(panel, PANEL_BUTTON,
	    PANEL_LABEL_X, value_x + 240,
	    PANEL_LABEL_Y, y,
	    PANEL_LABEL_IMAGE, 
		panel_button_image(panel, "MOVE IT", 0, image_font),
	    PANEL_NOTIFY_PROC, move_hp,
	    PANEL_SHOW_ITEM, FALSE,
	    0);
}


/* toggle the state of a disk */
static void
toggle_disk(disk)
Disk	disk;
{
    Panel_item	disk_item =(Panel_item)setup_get(disk, SETUP_OPAQUE, SETUP_ALL);
    Object_info	*info = (Object_info *) panel_get(disk_item, PANEL_CLIENT_DATA);
    Slot_info	*slot	= (Slot_info *) info->other;

    Hard_partition	hp;
    Panel_item		hp_item;
    register int	hp_index;
    register int	col;
    int			y;
    Hp_row		hp_row;

    if (slot) {	/* close it */
	/* hide the edit area */
	show_edit_area(0);

	/* hide the edit hp item */
	panel_set(edit_hp_item, PANEL_SHOW_ITEM, FALSE, 0);

	/* hide the data */
	SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, hp_index, hp)
	    hp_row = (Hp_row) setup_get(hp, SETUP_OPAQUE, SETUP_ALL);
	    FOREACH_COLUMN(col)
	        panel_set(hp_row[col], PANEL_SHOW_ITEM, FALSE, 0);
	    END_FOREACH
	SETUP_END_FOREACH

	/* hide the headings */
	FOREACH_COLUMN(col)
	    panel_set(slot->headings[col], PANEL_SHOW_ITEM, FALSE, 0);
	END_FOREACH

	/* hide the info area */
	show_info_area(0);

	/* hide the disk & show the original */
	panel_set(slot->open_disk, 
		  PANEL_SHOW_ITEM, FALSE, 
	          PANEL_LABEL_STRING, "",
		  0);
	glue(slot->open_disk, 0);

	glue(slot->closed_disk, disk);
	panel_set(slot->closed_disk, PANEL_SHOW_ITEM, TRUE, 0);

	slot->closed_disk = (Panel_item) 0;

	edit_disk = 0;

	return;
    }

    /* open it */
    if (!(slot = find_free_slot()))
       return;

    /* hide the original & show the table disk */
    slot->closed_disk = disk_item;
    glue(slot->closed_disk, 0);
    panel_set(slot->closed_disk, PANEL_SHOW_ITEM, FALSE, 0);

    /* give the open disk the same object info
     * as the closed disk, so we can call edit_disk_button().
     */
    glue(slot->open_disk, disk);
    panel_set(slot->open_disk, 
	      PANEL_LABEL_IMAGE, 
		  panel_get(slot->closed_disk, PANEL_LABEL_IMAGE),
	      PANEL_SHOW_ITEM, TRUE,
	      0);

    /* get the new hard partition list */
    get_hp_choices(disk, disk_info(INFO_HOG));

    /* show the info area */
    show_info_area(disk);

    /* show the headings */
    FOREACH_COLUMN(col)
	panel_set(slot->headings[col], PANEL_SHOW_ITEM, TRUE, 0);
    END_FOREACH

    /* show the table rows */
    y = slot->data_y;
    SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, hp_index, hp)
	show_hp_row(hp, slot->data_x, y);
	y += hp_y_height + HP_Y_GAP;
    SETUP_END_FOREACH

    /* get the new hard partition list */
    get_hp_choices(disk, edit_hp_item);

    /* show the edit hp item */
    panel_set(edit_hp_item, PANEL_SHOW_ITEM, TRUE, 0);

    edit_disk = disk;
}


static void
show_info_area(disk)
Disk	disk;
{
    Panel_item	item;

    edit_disk = disk;

    /* update the edit disk item */
    if (!disk)
	panel_set_value(edit_disk_item, 0);
    else
	panel_set_value(edit_disk_item, setup_get(ws, WS_DISK_INDEX, disk) + 1);

    /* show/hide the display units */
    panel_set(display_units, PANEL_SHOW_ITEM, disk, 0);

    FOREACH_INFO_ITEM(item)
	panel_set(item, PANEL_SHOW_ITEM, disk, PANEL_PAINT, PANEL_NONE, 0);
    END_FOREACH

    FOREACH_INFO_ITEM(item)
	glue(item, disk);
	panel_paint(item, PANEL_CLEAR);
    END_FOREACH
}


static void
show_hp_row(hp, x, y)
Hard_partition	hp;
register int	x, y;
{
    Hp_row	hp_row = (Hp_row) setup_get(hp, SETUP_OPAQUE, SETUP_ALL);
    register int	col;
    Panel_item		item;
    int			data_x;
    int			hp_offset;

    if (!hp_row) {
	hp_row = init_hp_row();
	setup_set(hp, SETUP_OPAQUE, SETUP_ALL, hp_row, 0);
    }

    FOREACH_COLUMN(col)
	item = hp_row[col];
	data_x = x + heading_info[col].data_offset;
	glue(item, hp);
	switch (col) {
	    default:
		panel_set(item, 
		    PANEL_ITEM_X, data_x,
		    PANEL_ITEM_Y, y,
		    PANEL_PAINT, PANEL_NONE,
		    0);
		break;

	    case HDG_PARTITION:
		panel_set(item, 
		    PANEL_ITEM_Y, y,
		    PANEL_PAINT, PANEL_NONE,
		    0);
		update_hp_offset(hp);
		break;
	}
	panel_set(item, PANEL_SHOW_ITEM, TRUE, 0);
	x += heading_info[col].length + HP_X_GAP;
    END_FOREACH
}

static void
show_edit_area(hp)
Hard_partition	hp;
{
    Hard_partition	old_edit_hp = edit_hp;
    Panel_item		item;

    edit_hp = hp;

    if (old_edit_hp)
	update_hp_image(old_edit_hp);

    if (!hp)
	panel_set_value(edit_hp_item, 0);
    else {
	update_hp_image(hp);
	panel_set_value(edit_hp_item, setup_get(hp, HARD_INDEX) + 1);
    }

    FOREACH_EDIT_ITEM(item)
	panel_set(item, 
	    PANEL_SHOW_ITEM, hp, 
	    PANEL_PAINT, PANEL_NONE,
	    0);
    END_FOREACH

    FOREACH_EDIT_ITEM(item)
	glue(item, hp);
	panel_paint(item, PANEL_CLEAR);
    END_FOREACH
}


/* utilities */
/* find an unused slot in the slot_info[] array.
 * Returns 0 if no free slots are available.
 */
static Slot_info *
find_free_slot()
{
    register int	index;
    Object_info		*info;

    for (index = 0; index < MAX_SLOTS; index++)
       /* if no closed disk, it's free */
       if (!slot_info[index].closed_disk)
	  return &slot_info[index];

    /* no free slots so close the open disk
     * in slot zero and use it.
     */
    info = (Object_info *) panel_get(slot_info[0].open_disk, PANEL_CLIENT_DATA);
    toggle_disk(info->obj);
    return &slot_info[0];
}


void
update_hp_image(hp)
Hard_partition	hp;
{

    Hp_row	hp_row	 = (Hp_row) setup_get(hp, SETUP_OPAQUE, SETUP_ALL);
    int		hp_width = (int) setup_get(hp, HARD_SIZE);
    char	*hp_name = (char *) setup_get(hp, HARD_LETTER);
    Pixrect	*hp_pr;
    int		width;
    Panel_item	item;

    if (!hp_row)
	return;

    item = hp_row[(int) HDG_PARTITION];
    hp_pr = (Pixrect *) panel_get(item, PANEL_LABEL_IMAGE);

    if (hp_pr)
       prs_destroy(hp_pr);
    
    width = hp_width / partition_scale(hp);
    if (width == 0)
	width = 4;
    hp_pr = image_string_width(hp_name, width);
    image_box(hp_pr, PIX_SRC);

    if (setup_get(hp, HARD_PARAM_FREEHOG))
       image_grey(hp_pr);
    if (hp == edit_hp)
	image_invert(hp_pr);

    panel_set(item, PANEL_LABEL_IMAGE, hp_pr, 0);
}


static int
partition_scale(hp)
Hard_partition	hp;
{
    return ((int) (setup_get(setup_get(hp, HARD_DISK), DISK_SIZE)) / 
            heading_info[(int)HDG_PARTITION].length);
}


void
update_units(disk)
Disk	disk;
{
    char	buffer[64], title[64];
    int		unit_index	= (int) setup_get(ws, PARAM_DISK_DISPLAY_UNITS);
    char	*units;
    register int	i, col;

    units = (char *) 
	setup_get(ws, SETUP_CHOICE_STRING, 
		  CONFIG_DISK_DISPLAY_UNITS, unit_index);

    for (i = 0; i < MAX_SLOTS; i++)
	for (col = ord(HDG_SIZE); col <= ord(HDG_SIZE); col++) {
	    sscanf(heading_info[col].title, "%[^(]", title);
	    sprintf(buffer, "%s (%s)", title, units);
	    panel_set(slot_info[i].headings[col],
		  PANEL_LABEL_STRING, buffer,
		  0);
	}
}


void
update_disk_overlap(disk)
Disk	disk;
{
    int	overlap_not_allowed;

    if (disk != edit_disk)
        return;

    overlap_not_allowed = !((int) setup_get(disk, DISK_PARAM_OVERLAPPING));

    panel_set(disk_info(INFO_FREE_LABEL), 
	PANEL_SHOW_ITEM, overlap_not_allowed,
	0);
    panel_set(disk_info(INFO_FREE), PANEL_SHOW_ITEM, overlap_not_allowed, 0);

    panel_set(disk_info(INFO_FLOAT), PANEL_SHOW_ITEM, overlap_not_allowed, 0);
    panel_set(disk_info(INFO_HOG), 
	PANEL_SHOW_ITEM, 
	    overlap_not_allowed && (int) setup_get(disk, DISK_PARAM_FLOATING),
	0);
}


void
update_hp_offset(hp)
Hard_partition	hp;
{
    Hp_row		hp_row	= (Hp_row) setup_get(hp, SETUP_OPAQUE, SETUP_ALL);
    int			new_offset;
    int			map_x = slot_info[0].map_x;

    if (!hp_row)
	return;

    new_offset = (int) setup_get(hp, HARD_OFFSET);
    panel_set(hp_row[(int) HDG_PARTITION],
	      PANEL_ITEM_X, map_x + new_offset / partition_scale(hp),
	      0);
}


void
update_hp_size(hp)
Hard_partition	hp;
{
    Panel_item		map;
    Hp_row		hp_row	= (Hp_row) setup_get(hp, SETUP_OPAQUE, SETUP_ALL);

    if (!hp_row)
       return;

    update_hp_image(hp);
    map	= hp_row[(int) HDG_PARTITION];
    panel_paint(map, PANEL_CLEAR);

    /* now update the size slider */
    update_hp_size_slider(hp);
}


void
update_hp_size_slider(hp)
Hard_partition	hp;
{
    if (hp != edit_hp)
	return;

    panel_set(edit_info(EDIT_SIZE_SLIDER), 
	PANEL_VALUE, atoi(setup_get(hp, HARD_SIZE_STRING)),
	PANEL_MIN_VALUE, setup_get(hp, HARD_MIN_SIZE_IN_UNITS),
	PANEL_MAX_VALUE, setup_get(hp, HARD_MAX_SIZE_IN_UNITS),
	0);
}

    

void
update_hp_type(hp)
Hard_partition	hp;
{
    int	move_allowed;

    if (hp != edit_hp)
	return;

    panel_set(edit_info(EDIT_MOUNT), 
	PANEL_SHOW_ITEM, ((Hard_type) setup_get(hp, HARD_TYPE)) == HARD_UNIX,
	0);

    move_allowed = (int) setup_get(hp, HARD_MOVEABLE);

    panel_set(edit_info(EDIT_MOVE_LABEL), PANEL_SHOW_ITEM, move_allowed, 0);

    panel_set(edit_info(EDIT_MOVE_WHAT),
	PANEL_LABEL_STRING, setup_get(hp, HARD_WHAT_IT_IS),
        PANEL_SHOW_ITEM, move_allowed,
	0);

    panel_set(edit_info(EDIT_MOVE_TO), PANEL_SHOW_ITEM, move_allowed, 0);
    panel_set(edit_info(EDIT_MOVE_DO), PANEL_SHOW_ITEM, move_allowed, 0);
}



/* notify procs */

static void
edit_disk_button(item, event)
Panel_item	item;
Event		*event;
{
    Object_info	*info	= (Object_info *) panel_get(item, PANEL_CLIENT_DATA);

    if (item == disk_info(INFO_CLOSE))
	toggle_disk(edit_disk);
    else
	toggle_disk((Disk) info->obj);
}


/* edit the disk represented by
 * choice item.
 */
static void
edit_disk_choice(item, value, event)
Panel_item	item;
int		value;
Event		*event;
{
    register int	cont_index, disk_index, abs_disk_index;
    Controller		cont;
    Disk		disk;

    if (value == 0) {
	if (edit_disk)
	    toggle_disk(edit_disk);
	return;
    }

    value--;		/* disk index desired is value - 1 */
    abs_disk_index = 0;
    SETUP_FOREACH_OBJECT(ws, WS_CONTROLLER, cont_index, cont)
	SETUP_FOREACH_OBJECT(cont, CONTROLLER_DISK, disk_index, disk)
	    if (abs_disk_index++ == value) {
		if (disk != edit_disk)
		    toggle_disk(disk);
		return;
	    }
	SETUP_END_FOREACH
    SETUP_END_FOREACH
}



/* edit the hard partition represented by
 * button item.
 */
static void
edit_hp_button(item, event)
Panel_item	item;
Event		*event;
{
    Object_info	*info	= (Object_info *) panel_get(item, PANEL_CLIENT_DATA);

    if (item == edit_info(EDIT_CLOSE))
	show_edit_area(0);
    else
	show_edit_area(info->obj);
}


/* edit the hard partition represented by
 * choice item.
 */
static void
edit_hp_choice(item, value, event)
Panel_item	item;
int		value;
Event		*event;
{
    if (value == 0)
	show_edit_area(0);
    else
	show_edit_area(setup_get(edit_disk, DISK_HARD_PARTITION, value - 1));
}


static void
adjust_hp_size(item, value, event)
Panel_item	item;
int		value;
Event		*event;
{
    char	value_string[64];

    sprintf(value_string, "%d", value);
    panel_set_value(edit_info(EDIT_SIZE), value_string);
}


static void
move_hp(item, event)
Panel_item	item;
Event		*event;
{
    setup_set(ws, 
	WS_MOVE_HARD_PART, 
	    edit_hp, panel_get_value(edit_info(EDIT_MOVE_TO)),
	0);
}


/* event proc */
static void
set_hp_size(item, event)
Panel_item	item;
Event		*event;
{
    panel_default_handle_event(item, event);

    switch (event_id(event)) {
	case PANEL_EVENT_CANCEL:
	    panel_set_value(edit_info(EDIT_SIZE), 
		setup_get(edit_hp, HARD_SIZE_STRING_LEFT));
	    break;

	case MS_LEFT:
	    if (event_is_down(event))
		break;

	    setup_set(edit_hp,
		HARD_SIZE_STRING_LEFT, 
		    panel_get_value(edit_info(EDIT_SIZE)),
		0);
	    break;
    }
}


static void
get_hp_choices(disk, item)
Disk		disk;
Panel_item	item;
{
    register int		hp_index;
    register Hard_partition	hp;

    panel_set(item, PANEL_VALUE, 0, PANEL_PAINT, PANEL_NONE, 0);

    SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, hp_index, hp)
        panel_set(item, 
	    PANEL_CHOICE_STRING, 
		hp_index + 1, setup_get(hp, HARD_LETTER),
	    PANEL_PAINT, PANEL_NONE,
	    0);
    SETUP_END_FOREACH
}



static void
get_move_to_choices(item)
Panel_item	item;
{
    register int	cont_index, disk_index, hp_index, abs_hp_index;
    Controller		cont;
    Disk		disk;
    Hard_partition	hp;

    abs_hp_index = 0;
    SETUP_FOREACH_OBJECT(ws, WS_CONTROLLER, cont_index, cont)
	SETUP_FOREACH_OBJECT(cont, CONTROLLER_DISK, disk_index, disk)
	    SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, hp_index, hp)
		panel_set(item, 
		    PANEL_CHOICE_STRING, 
			abs_hp_index++, setup_get(hp, HARD_NAME),
		    PANEL_PAINT, PANEL_NONE,
		    0);
	    SETUP_END_FOREACH
	SETUP_END_FOREACH
    SETUP_END_FOREACH
}

#ifndef lint
static	char sccsid[] = "@(#)tty_edit_disk.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "tty_global.h"
#include "tty_item.h"

/* 
 * edit_disk_init() - creates items and their positions.
 * 
 */


#define WHAT_IS_START	0
/* 1st 4 is the number of characters required by the choice item; 2nd number 
 * is the number of characters in the string.
 */
#define WHAT_IS_SIZE	(4 + 18)
#define SIZE_START	(WHAT_IS_START + WHAT_IS_SIZE + 1)
#define SIZE_SIZE	10
#define GRAPHICS_START	(SIZE_START + SIZE_SIZE + 1)
#define GRAPHICS_SIZE	(COLS - GRAPHICS_START - 2)	
 
 
static	Item	edit_disk_item;
static	Item	close_disk_item;
static	Item	cyl_rounding_item;
static	Item	overlap_item;
static	Item		float_item;
static	Item			free_hog_item;

static	char	label[] = "   Hard Partition        Size:";
static	Item	unit_item;
static	Item	disk_size_item;
static	Item	disk_free_item;
static	Item	hp_map_item[SETUP_MAX_HARD_PARTITIONS];
static	Item	hp_list_string_item[SETUP_MAX_HARD_PARTITIONS];
static	Item	hp_list_size_item[SETUP_MAX_HARD_PARTITIONS];

static	Item	edit_hp_item;
static	Item	close_hp_item;
static	Item	offset_item;
static	Item	size_item;
static	Item	type_item;
static	Item	mount_pt_item;
static	Item	move_what_it_is_item;
static	Item	move_to_item;
static	Item	move_do_item;

static	Disk	cur_disk;
static	int	cur_disk_num;
static	Hard_partition	cur_hp = NULL;
static	int	cur_hp_index;

static	int	generate_all_hp_name_strings();
static	Disk	find_disk();	
static	int	units_notify_proc();
static	int	calculate_units_string();
static	int	undo_disk_glue();
static	int	draw_hp_info();
static	int	draw_hp_list_element();
static	int	hp_callback_proc();
static	int	edit_disk_notify_proc();	
static	int	edit_hp_notify_proc();	
static	int	fill_in_hp_list();
static	int	fill_in_cur_hp_info();
static	int	fill_in_cur_disk_info();
static	int	get_free_hog_choice_strings();
static	int	get_free_hog();
static	int	set_free_hog_notify_proc();
static	int	draw_free_hog_info();
static	int	draw_float_info();
static	int	free_hog_draw();
static	int	float_draw();
static	int	check_overlap_notify_proc();
static	int	float_notify_proc();
static	int	move_do_notify_proc();
static	int	hp_type_notify_proc();
static	int	hp_type_draw();

extern	Workstation	ws;
extern	Form		edit_disk_form;
	
	
tty_edit_disk_init()
{   
    register	int	i, j; 
    		Item	item;
		char	buff[5];

    edit_disk_item = CHOICE_ITEM(edit_disk_form, 0, 0, "Edit Disk:", -1);
    form_item_set(edit_disk_item, ITEM_CHOICE_STRING, "NONE", 0);
    tty_get_disk_names(edit_disk_item);
    form_item_set(edit_disk_item,
		  ITEM_NOTIFY_PROC, edit_disk_notify_proc,
		  ITEM_CHOICE_CYCLE_DISPLAY, 	TRUE, 
		  0);
    
    form_set(edit_disk_form, FORM_CURRENT_IO_ITEM,edit_disk_item, 0);
    
    close_disk_item = BUTTON_ITEM(edit_disk_form, 0, 35, "Close");
    form_item_set(close_disk_item, ITEM_NOTIFY_PROC, edit_disk_notify_proc, 0);
    
    cyl_rounding_item = CHOICE_ITEM(edit_disk_form, 0, 47, "Round to Cylinders:", 70);
    form_item_set(cyl_rounding_item, 
		  ITEM_CHOICE_CYCLE_DISPLAY, 	TRUE, 
		  0);
    tty_yes_no_choice(cyl_rounding_item);
    
    overlap_item = CHOICE_ITEM(edit_disk_form, 1, 47, "Overlapping Allowed:", 70);
    form_item_set(overlap_item, 
		  ITEM_NOTIFY_PROC, check_overlap_notify_proc, 
		  ITEM_CHOICE_CYCLE_DISPLAY, 	TRUE, 
		  0);
    tty_yes_no_choice(overlap_item);
    
    float_item = CHOICE_ITEM(edit_disk_form, 2, 47, "Float:", 70);
    form_item_set(float_item, 
		  ITEM_NOTIFY_PROC, float_notify_proc, 
		  ITEM_CHOICE_CYCLE_DISPLAY, 	TRUE, 
		  0);
    tty_yes_no_choice(float_item);
    
    free_hog_item = CHOICE_ITEM(edit_disk_form, 3, 47,"Free Space Hog:", 70);
    form_item_set(free_hog_item, 
		  ITEM_NOTIFY_PROC, 		set_free_hog_notify_proc, 
		  ITEM_CHOICE_CYCLE_DISPLAY, 	TRUE, 
		  0);
    disk_size_item = 
	TEXT_ITEM_OO(edit_disk_form, 1, 0, "Total Size:", -1, 10);
    disk_free_item = 
	TEXT_ITEM_OO(edit_disk_form, 1, 25, "Free:", -1, 10);
    
    /* 
     * Label for HP list, units and disk size and amount free.
     */
    unit_item =
	TEXT_ITEM_OO(edit_disk_form, 4, 0, label, -1, 12);
				
    for(i = 0,  j = 5; i < SETUP_MAX_HARD_PARTITIONS; i++, j++) {
	sprintf(buff, "%c", ('a' + i));
	hp_list_string_item[i] = 
	    TEXT_ITEM_OO(edit_disk_form, j, WHAT_IS_START, buff, -1, WHAT_IS_SIZE);
	hp_list_size_item[i] = 
	    TEXT_ITEM_OO(edit_disk_form, j, SIZE_START, "", -1, SIZE_SIZE);
	hp_map_item[i] = 
	 TEXT_ITEM_OO(edit_disk_form, j, GRAPHICS_START, "", -1, GRAPHICS_SIZE + 1);
    }



    /* 
     * Hard partition editting.
     * 
     */
    
    edit_hp_item = CHOICE_ITEM(edit_disk_form, 14,  0, "Edit Hard Partition:", -1);
    form_item_set(edit_hp_item, 
		  ITEM_NOTIFY_PROC, 		edit_hp_notify_proc, 
		  ITEM_CHOICE_CYCLE_DISPLAY, 	TRUE, 
		  0);
    
    close_hp_item = BUTTON_ITEM(edit_disk_form, 14,  40, "Close");
    form_item_set(close_hp_item, 
		  ITEM_NOTIFY_PROC, 		edit_hp_notify_proc, 
		  0);
    
    offset_item  = TEXT_ITEM(  edit_disk_form, 15,  4, "Offset:", 14,	10);
    size_item    = TEXT_ITEM(  edit_disk_form, 15, 30, "Size:",   43, 	10);
    type_item    = CHOICE_ITEM(edit_disk_form, 16,  4, "Type:",   14);
    form_item_set(type_item, 
		  ITEM_CHOICE_CYCLE_DISPLAY, 	TRUE, 
		  0);
    glue_choices(type_item, ws, CONFIG_HARD_PARTITION_TYPE);
    mount_pt_item = TEXT_ITEM(edit_disk_form, 16, 30, "Mount Point:", 43,(COLS-37-1));

    move_what_it_is_item = 
	TEXT_ITEM_OO(edit_disk_form, 17, 4, "Move:", 14, 15);
    
    move_to_item = CHOICE_ITEM(edit_disk_form, 17, 30, "To:", 43);
    generate_all_hp_name_strings(move_to_item);
    form_item_set(move_to_item, 
		  ITEM_CHOICE_CYCLE_DISPLAY,	TRUE, 
		  0);

    move_do_item = BUTTON_ITEM(edit_disk_form, 17, 55, "MOVE IT");
    form_item_set(move_do_item, ITEM_NOTIFY_PROC, move_do_notify_proc, 0);
    
    cur_disk_num = 0;
    cur_disk = find_disk(cur_disk_num);
    cur_hp_index = 0;
    cur_hp = NULL;
    draw_disk_info(FALSE);
    draw_hp_info(FALSE);
    
}


static
int
generate_all_hp_name_strings(item)
register	Item	item;
{
    Controller		cont;
    int			cont_index;
    Disk		disk;
    int			disk_index;
    Hard_partition	hp;
    int			hp_index;
    
    form_item_set(item, ITEM_CHOICE_RESET, 0);
    SETUP_FOREACH_OBJECT(ws, WS_CONTROLLER, cont_index, cont) {
	SETUP_FOREACH_OBJECT(cont, CONTROLLER_DISK, disk_index, disk) {
	    SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, hp_index, hp) {
		form_item_set(item, 
			      ITEM_CHOICE_STRING, setup_get(hp, HARD_NAME), 
			      0);
	    }  SETUP_END_FOREACH /* disk */
	}  SETUP_END_FOREACH /* disk */
    } SETUP_END_FOREACH /* controller */
}



static
Disk
find_disk(requested_disk_num)
register	int	requested_disk_num;
{   
    register	int			disk_num;
    Controller		cont;
    int			cont_index;
    Disk			disk;
    int			disk_index;
    
    if (requested_disk_num == 0)
	return((Disk)NULL);
    else
	requested_disk_num--;
    
    disk_num = 0;
    SETUP_FOREACH_OBJECT(ws, WS_CONTROLLER, cont_index, cont) {
	SETUP_FOREACH_OBJECT(cont, CONTROLLER_DISK, disk_index, disk) {
	    if (disk_num++ == requested_disk_num)
		return(disk);
	}  SETUP_END_FOREACH /* disk */
    } SETUP_END_FOREACH /* controller */
    
    return((Disk)NULL);
}


static
int
find_hp(item, hp, hp_index)
Item		item;
Hard_partition	*hp;
int		*hp_index;
{   
    register	int	i;
    
    i = (int) form_item_get(item, ITEM_VALUE);
    if (i == 0)
	*hp = NULL;
    else
	*hp = setup_get(cur_disk, DISK_HARD_PARTITION, (i - 1));
    
    *hp_index = i - 1 ;
}


tty_get_disk_names(item)
Item	item;
{   
    Controller          cont;
    int                 cont_index;
    Disk                disk;
    int                 disk_index;
    
    SETUP_FOREACH_OBJECT(ws, WS_CONTROLLER, cont_index, cont) {
	SETUP_FOREACH_OBJECT(cont, CONTROLLER_DISK, disk_index, disk) {
	    form_item_set(item, 
			  ITEM_CHOICE_STRING,  setup_get(disk, DISK_NAME), 
			  0);
	}  SETUP_END_FOREACH /* disk */
    } SETUP_END_FOREACH /* controller */
}


static
int
get_free_hog_choice_strings(item)
Item	item;
{   
    register	Hard_partition	hp;
    register	int		hp_index;
    
    form_item_set(item, ITEM_CHOICE_RESET, 0);
    form_item_set(item, ITEM_CHOICE_STRING, "None", 0);
    SETUP_FOREACH_OBJECT(cur_disk, DISK_HARD_PARTITION, hp_index, hp) {
	form_item_set(item, 
		      ITEM_CHOICE_STRING, setup_get(hp, HARD_LETTER), 
		      0);
    } SETUP_END_FOREACH;
}

static
int
get_free_hog(item)
Item	item;
{   
    
    register	Hard_partition	hp;
    register	int		hp_index;
    
    SETUP_FOREACH_OBJECT(cur_disk, DISK_HARD_PARTITION, hp_index, hp) {
	if ((int)setup_get(hp, HARD_PARAM_FREEHOG) == TRUE) {
	    form_item_set(item, ITEM_VALUE, (hp_index + 1), 0);
	    return;
	}
    } SETUP_END_FOREACH;
    form_item_set(item, ITEM_VALUE, 0, 0);	/* value of 0 == "None" */
}



static
int
set_free_hog_notify_proc(item)
Item	item;
{   
    register	int	hp_index;
    register	Hard_partition	hp;
    
    hp_index = (int) form_item_get(item, ITEM_VALUE);
    setup_set(cur_disk, 
	      DISK_FREE_HOG_INDEX, hp_index, 
	      0);
    
    return(FALSE);
}
	      

static
int
draw_free_hog_info(val)
register	int	val;
{   
    form_item_set(free_hog_item, ITEM_DISPLAYED, val, 0);
    form_item_display(free_hog_item);
}


static
int
draw_float_info(val)
register	int	val;
{   
    form_item_set(float_item, ITEM_DISPLAYED, val, 0);
    form_item_set(disk_free_item, ITEM_DISPLAYED, val, 0);
    form_item_display(float_item);
    form_item_display(disk_free_item);
}




static
int
free_hog_draw()
{   
    if ((int)form_item_get(float_item, ITEM_VALUE) == TRUE)
	draw_free_hog_info(TRUE);
    else
	draw_free_hog_info(FALSE);
    
}

static
int
float_draw()
{   
    if ((int)form_item_get(overlap_item, ITEM_VALUE) == FALSE) {
	draw_float_info(TRUE);
	free_hog_draw();
    }
    else {
	draw_float_info(FALSE);
	draw_free_hog_info(FALSE);
    }
}


static
int
check_overlap_notify_proc(item, ch)
Item	item;
int	ch;
{   
    glue_notify_proc(item, ch);
    float_draw();
}
	
static
int
float_notify_proc(item, ch)
Item	item;
int	ch;
{   
    glue_notify_proc(item, ch);
    free_hog_draw();
}
	




tty_edit_disk_pre_display()
{   
    fill_in_cur_disk_info();
    fill_in_cur_hp_info();
    calculate_units_string();
}



static
int
edit_disk_notify_proc(item, ch)
Item	item;
int	ch;
{    
    
    
    undo_disk_glue();    
    if (item == close_disk_item) {
	cur_disk = NULL;
    }
    else {
	cur_disk = find_disk((int)form_item_get(item, ITEM_VALUE));	
    }
	
    if (cur_disk == (Disk)NULL) {
	draw_disk_info(FALSE);
	form_item_set(edit_disk_item, ITEM_VALUE, 0, 0);
	form_item_display(edit_disk_item);
    }
    else {
	fill_in_cur_disk_info();
	glue_disk();
	draw_disk_info(TRUE);
    }

    form_item_set(edit_hp_item, ITEM_VALUE, 0, 0);
    edit_hp_notify_proc(edit_hp_item, '\n');
    
    form_set(edit_disk_form, FORM_REFRESH, 0);        
    return(FALSE);
}

static
int
glue_disk()
{   
    if (cur_disk == NULL)
	return;
    
    glue(cyl_rounding_item, cur_disk, DISK_PARAM_CYLROUNDING, (Item)NULL);
    
    glue(overlap_item, cur_disk, DISK_PARAM_OVERLAPPING, (Item)NULL);
    form_item_set(overlap_item, ITEM_NOTIFY_PROC, check_overlap_notify_proc, 0);
    
    glue(float_item,	 cur_disk, DISK_PARAM_FLOATING,		(Item)NULL); 
    form_item_set(float_item, ITEM_NOTIFY_PROC, float_notify_proc, 0);
    
    glue(free_hog_item, cur_disk, DISK_FREE_HOG_INDEX, (Item)NULL);
    
    glue(disk_size_item, cur_disk, DISK_SIZE_STRING_LEFT, (Item)NULL);
	 
    glue(disk_free_item, cur_disk, DISK_FREE_SPACE_STRING_LEFT, (Item)NULL);
    
    setup_set(cur_disk, SETUP_CALLBACK, glue_callback_proc, 0);
}


static
int
undo_disk_glue()
{   
    register	Hard_partition	hp;
    register	int		hp_index;
    
    if (cur_disk == (Disk)NULL)
	return;
    
    setup_set(cur_disk, SETUP_OPAQUE, DISK_PARAM_CYLROUNDING, (Item)NULL, 0);
    setup_set(cur_disk, SETUP_OPAQUE, DISK_PARAM_OVERLAPPING, (Item)NULL, 0);
    setup_set(cur_disk, SETUP_OPAQUE, DISK_PARAM_FLOATING,    (Item)NULL, 0);
    setup_set(cur_disk, SETUP_OPAQUE, DISK_FREE_HOG_INDEX,    (Item)NULL, 0);
    setup_set(cur_disk, SETUP_OPAQUE, DISK_SIZE_STRING_LEFT,  (Item)NULL, 0);
    setup_set(cur_disk, SETUP_OPAQUE, DISK_FREE_SPACE_STRING_LEFT,(Item)NULL,0);
    
}



static
int
fill_in_cur_disk_info()
{   
    register	Hard_partition	hp;
    register	int		hp_index;
    register	int		i;
    
    if (cur_disk == NULL)
	return;
    
    form_item_set(overlap_item, 
		  ITEM_VALUE, (int) setup_get(cur_disk, DISK_PARAM_OVERLAPPING),
		  0);
    form_item_set(float_item, 
		  ITEM_VALUE, (int) setup_get(cur_disk, DISK_PARAM_FLOATING), 
		  0);
    get_free_hog_choice_strings(free_hog_item);
/*
 * Now glueing DISK_FREE_HOG_INDEX so this line is not neeed.
 *   get_free_hog(free_hog_item); 
 */
    
    float_draw();
    
    form_item_set(edit_hp_item, ITEM_CHOICE_RESET, 0);
    form_item_set(edit_hp_item, ITEM_CHOICE_STRING, "NONE", 0);
    SETUP_FOREACH_OBJECT(cur_disk, DISK_HARD_PARTITION, hp_index, hp) {
	fill_in_hp_list(hp, hp_index);
	setup_set(hp, SETUP_CALLBACK, hp_callback_proc, 0);
	form_item_set(edit_hp_item, 
		      ITEM_CHOICE_STRING, setup_get(hp, HARD_LETTER), 
		      0);
	
    } SETUP_END_FOREACH;
    
}


static
int
fill_in_hp_list(hp, hp_index)
register	Hard_partition	hp;
register	int		hp_index;
{  
    char		buff[140];
    int			start, end;
    double		offset, hp_size, disk_size;
    register	int	i;
    char		line_char;
    
    
    form_item_set(hp_list_string_item[hp_index], 
		  ITEM_VALUE,  (char *) setup_get(hp, HARD_WHAT_IT_IS), 
		  0);
    form_item_set(hp_list_size_item[hp_index], 
		  ITEM_VALUE,  (char *) setup_get(hp, HARD_SIZE_STRING), 
		  0);

    line_char = ((int)setup_get(hp, HARD_PARAM_FREEHOG) ? '=' : '-');
    
    i = (int) setup_get(cur_disk, DISK_SIZE);    
    disk_size = (double)i;
    i = (int) setup_get(hp, HARD_OFFSET);
    offset = (double) i;
    i = (int) setup_get(hp, HARD_SIZE);
    hp_size = (double) i;
    if (hp_size != 0.0) {
	start = (int) ((GRAPHICS_SIZE) * (offset / disk_size));
	end = (int) ((GRAPHICS_SIZE) * ((offset + hp_size) / disk_size));
	for (i = 0; i < start; i++) 
	    buff[i] = ' ';
	buff[start] = '|';
	for (i = start + 1; i < end; i++) 
	    buff[i] = line_char;
	buff[end] = '|';
	buff[end + 1] = '\0';
	form_item_set(hp_map_item[hp_index], 
		      ITEM_VALUE, buff, 
		      0);
    }
    else {
	form_item_set(hp_map_item[hp_index], 
		      ITEM_VALUE, "", 
		      0);
    }
}



static
int
draw_disk_info(val)
int	val;
{   
    register	int		i;

    for ( i = 0; i < SETUP_MAX_HARD_PARTITIONS; i++) {
	draw_hp_list_element(i, val);
    }

    form_item_set(close_disk_item, ITEM_DISPLAYED, val, 0);
    form_item_set(edit_hp_item,	ITEM_DISPLAYED, val, 0);
    form_item_set(overlap_item,	ITEM_DISPLAYED, val, 0);
    form_item_set(cyl_rounding_item, ITEM_DISPLAYED, val, 0);
    form_item_set(unit_item,	ITEM_DISPLAYED, val, 0);
    
    if (val == FALSE) {
	form_item_set(disk_free_item,	ITEM_DISPLAYED, val, 0);
	form_item_set(float_item,	ITEM_DISPLAYED, val, 0);
	form_item_set(free_hog_item,	ITEM_DISPLAYED, val, 0);
    }
    form_item_set(disk_size_item,ITEM_DISPLAYED, val, 0);
    
    form_item_display(edit_hp_item);
    form_item_display(close_disk_item);
    form_item_display(cyl_rounding_item);
    form_item_display(overlap_item);
    form_item_display(float_item);
    form_item_display(free_hog_item);
    form_item_display(unit_item);
    form_item_display(disk_size_item);
    form_item_display(disk_free_item);
    
}

static
int
draw_hp_list_element(hp_index, val)
register	int	hp_index;
register	int	val;
{   
    form_item_set(hp_list_string_item[hp_index],ITEM_DISPLAYED, val, 0);
    form_item_set(hp_list_size_item[hp_index],	ITEM_DISPLAYED, val, 0);
    form_item_set(hp_map_item[hp_index],	ITEM_DISPLAYED, val, 0);
    
    form_item_display(hp_list_string_item[hp_index]);
    form_item_display(hp_list_size_item[hp_index]);
    form_item_display(hp_map_item[hp_index]);
}
    


static
int
edit_hp_notify_proc(item, ch)
Item	item;
int	ch;
{   
    register	int	i, j;
    
    undo_hp_glue();
    
    if (item == close_hp_item) {
	cur_hp = (Hard_partition) NULL;
	cur_hp_index = 0;
	form_item_set(edit_hp_item, ITEM_VALUE, 0, 0);
	form_item_display(edit_hp_item);
    }
    else
	find_hp(item, &cur_hp, &cur_hp_index);
    if (cur_hp == NULL) {
	draw_hp_info(FALSE);
    }
    else {
	glue_hp();
	glue_notify_proc(item, ch);
	draw_hp_info(TRUE);
    }
    form_set(edit_disk_form, FORM_REFRESH, 0);    
    return(FALSE);
}
 


static
int
draw_move_info(val)
register	int	val;
{   
    form_item_set(move_what_it_is_item, ITEM_DISPLAYED, val, 0);
    form_item_set(move_to_item, ITEM_DISPLAYED, val, 0);
    form_item_set(move_do_item, ITEM_DISPLAYED, val, 0);

    form_item_display(move_what_it_is_item);
    form_item_display(move_to_item);
    form_item_display(move_do_item);
    
}


static
int
hp_type_draw()
{   
    if ((int)setup_get(cur_hp, HARD_MOVEABLE)) {
	glue(move_what_it_is_item, cur_hp, HARD_WHAT_IT_IS, (Item)NULL);
	draw_move_info(TRUE);
    }
    else 
	draw_move_info(FALSE);
    
    if ((Hard_type)setup_get(cur_hp, HARD_TYPE) == HARD_UNIX) {
	glue(mount_pt_item, cur_hp, HARD_MOUNT_PT, (Item)NULL);
	form_item_set(mount_pt_item, ITEM_DISPLAYED, TRUE, 0);
    }
    else
	form_item_set(mount_pt_item, ITEM_DISPLAYED, FALSE, 0);
    form_item_display(mount_pt_item);
}



static
int
glue_hp()
{   
    glue(offset_item,		cur_hp, HARD_OFFSET_STRING_LEFT,(Item)NULL);
    glue(size_item, 		cur_hp, HARD_SIZE_STRING_LEFT,	(Item)NULL);
    glue(type_item,		cur_hp, HARD_TYPE,		(Item)NULL);
    form_item_set(type_item, ITEM_NOTIFY_PROC, hp_type_notify_proc, 0);

    hp_type_draw();
}


static
int
undo_hp_glue()
{   
    
    if (cur_hp == (Hard_partition)NULL)
	return;
    
    setup_set(cur_hp, SETUP_OPAQUE, HARD_OFFSET_STRING_LEFT, (Item)NULL, 0);
    setup_set(cur_hp, SETUP_OPAQUE, HARD_SIZE_STRING_LEFT,   (Item)NULL, 0);
    setup_set(cur_hp, SETUP_OPAQUE, HARD_TYPE, 		     (Item)NULL, 0);
    
}

static
int
draw_hp_info(val)
int	val;
{   
    form_item_set(close_hp_item, ITEM_DISPLAYED, val, 0);
    form_item_set(offset_item, ITEM_DISPLAYED, val, 0);
    form_item_set(size_item, ITEM_DISPLAYED, val, 0);
    form_item_set(type_item, ITEM_DISPLAYED, val, 0);
    if (val == FALSE) {
	form_item_set(mount_pt_item, ITEM_DISPLAYED, val, 0);
	form_item_set(move_what_it_is_item, ITEM_DISPLAYED, val, 0);
	form_item_set(move_to_item, ITEM_DISPLAYED, val, 0);
	form_item_set(move_do_item, ITEM_DISPLAYED, val, 0);
    }
    form_item_display(close_hp_item);
    form_item_display(offset_item);
    form_item_display(size_item);
    form_item_display(type_item);
    form_item_display(mount_pt_item);
    form_item_display(move_what_it_is_item);
    form_item_display(move_to_item);
    form_item_display(move_do_item);
}

static
int
fill_in_cur_hp_info()
{ 
    if ((cur_disk == NULL) || (cur_hp == NULL))
	return;
    
    form_item_set(offset_item, 
		  ITEM_VALUE, setup_get(cur_hp, HARD_OFFSET_STRING_LEFT), 
		  0);
    form_item_set(size_item, 
		  ITEM_VALUE, setup_get(cur_hp, HARD_SIZE_STRING_LEFT), 
		  0);
    form_item_set(type_item, 
		  ITEM_VALUE, setup_get(cur_hp, HARD_TYPE), 
		  0);
    form_item_set(mount_pt_item, 
		  ITEM_VALUE, setup_get(cur_hp, HARD_MOUNT_PT), 
		  0);
    
}


static
int
hp_callback_proc(obj, attr, display_value, err_msg)
Opaque          obj;
Setup_attribute attr;
caddr_t         display_value;
char            *err_msg;
{
    register	Hard_partition	hp;
    register	int		hp_index;
    register	int		found_it;
    
    if (cur_disk == NULL)
	return;
    
    found_it = FALSE;
    SETUP_FOREACH_OBJECT(cur_disk, DISK_HARD_PARTITION, hp_index, hp) {
	if (hp == (Hard_partition) obj) {
	    found_it = TRUE;
	    break;
	}
    } SETUP_END_FOREACH;
    if (!found_it)
	return; /*the hp is not on the currently displayed disk so ignore it*/
    
    fill_in_hp_list(hp, hp_index);
    draw_hp_list_element(hp_index);
    form_set(edit_disk_form, FORM_REFRESH, 0);
    glue_callback_proc(obj, attr, display_value, err_msg);
}



static
int
move_do_notify_proc(item, ch)
Item	item;
int	ch;
{   
    extern	int	callback_error;
    
    setup_set(ws, WS_MOVE_HARD_PART, cur_hp, 
	             (int)form_item_get(move_to_item, ITEM_VALUE), 0);
    if (!callback_error) {
	draw_move_info(FALSE);
	form_set(edit_disk_form, FORM_REFRESH, 0);
    }
    form_set(edit_disk_form, FORM_REFRESH, 0);    
    return(FALSE);
}


static
int
hp_type_notify_proc(item, ch)
Item	item;
int	ch;
{   
    setup_set(cur_hp, HARD_TYPE, (Hard_type)form_item_get(type_item, ITEM_VALUE), 0);
    
    hp_type_draw();
    return(FALSE);
}



static
int
calculate_units_string()
{   
    register	int	num;
    register	char	*str;
    
    num = (int) setup_get(ws, PARAM_DISK_DISPLAY_UNITS);
    str = (char *) setup_get(ws, 
	     SETUP_CHOICE_STRING, CONFIG_DISK_DISPLAY_UNITS, num);
    
    form_item_set(unit_item, ITEM_VALUE, str, 0);
}

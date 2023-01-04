
/*	@(#)setup.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sunwindow/window_hs.h>
#include <suntool/tool.h>
#include <suntool/icon.h>
#include <suntool/panel.h>

#include <setup_attr.h>
#include <setup_message_table.h>


/* macros */

#define new(type)       ((type *) calloc(sizeof(type), 1))

#define	object_info(obj, attr)	object_info_plus(obj, attr, 0)

#define	image_string(string)	image_string_width(string, 0)

#define	CYCLE_ATTRS(x, y, value_offset)	\
    PANEL_LABEL_X, 		x,		\
    PANEL_LABEL_Y, 		y,		\
    PANEL_MARK_XS, 		x + value_offset - 22, 0,	\
    PANEL_MARK_YS, 		y + 4, 0,		\
    PANEL_CHOICE_XS, 		x + value_offset, 0, 	\
    PANEL_CHOICE_YS, 		y + 4, 0,		\
    PANEL_MARK_IMAGES, 		image_cycle, 0, \
    PANEL_NOMARK_IMAGES, 	0,		\
    PANEL_FEEDBACK, 		PANEL_MARKED,	\
    PANEL_DISPLAY_LEVEL, 	PANEL_CURRENT


/* structs */

typedef struct {
    Opaque		obj;
    Setup_attribute	attr;
    Opaque		other;
} Object_info;


/* global variables */

extern Tool		*tool;

extern Workstation	ws;
extern Rect		display_rect;

/* navigation buttons for show_screen() */

extern Panel_item	machine_screen;
extern Panel_item	disk_screen;
extern Panel_item	client_screen;
extern Panel_item	soft_part_screen;
extern Panel_item	software_screen;
extern Panel_item	defaults_screen;


/* Font used for image and panel text strings */
extern Pixfont		*image_font;
extern Pixfont		*text_font;

/* Common images */
extern Pixrect		*image_modify;
extern Pixrect		*image_add;
extern Pixrect		*image_delete;
extern Pixrect		*image_apply;
extern Pixrect		*image_close;
extern Pixrect		*image_status;
extern Pixrect		*image_help;
extern Pixrect		*image_done;
extern Pixrect		*image_exit;
extern Pixrect		*image_execute;
extern Pixrect		*image_reboot;
extern Pixrect		*image_machine;
extern Pixrect		*image_disks;
extern Pixrect		*image_clients;
extern Pixrect		*image_soft_parts;
extern Pixrect		*image_config_cards;
extern Pixrect		*image_software;
extern Pixrect		*image_defaults;
extern Pixrect		*image_disk;
extern Pixrect		*image_controller1;
extern Pixrect		*image_controller2;
extern Pixrect		*image_sd_disk;
extern Pixrect		*image_sd_controller1;
extern Pixrect		*image_sd_controller2;
extern Pixrect		*image_xy_disk;
extern Pixrect		*image_xy_controller1;
extern Pixrect		*image_xy_controller2;
extern Pixrect		*image_hardpart;
extern Pixrect		*image_client;
extern Pixrect		*image_card;
extern Pixrect		*image_ok;
extern Pixrect		*image_no;
extern Pixrect		*image_yes;
extern Pixrect		*image_cycle;

/* global functions */

/*VARARGS1*/
extern void		setup_error();

extern Panel		make_panel();
extern void		show_panel();
extern void		show_screen();
extern void		message_print();
extern void		message_repaint();
extern int		confirm_yes_no();
extern void		confirm_ok();

extern void		get_choices();
extern Panel_setting	set_text_info();
extern Panel_setting	set_caret();
extern void		set_info();
extern void		get_object_info();
extern void		glue();
extern Object_info	*object_info_plus();
extern void		handle_error();

extern Pixrect		*image_label_pr();
extern Pixrect		*image_string_width();
extern void		image_relabel_pr();
extern void		image_box();
extern void		image_invert();
extern void		image_grey();

extern void		update_ws_type();
extern void		update_tape_location();
extern void		update_e_board_type();
extern void		update_yp_type();
extern void		update_autohost();
extern void		update_units();
extern void		update_disk_overlap();
extern void		update_hp_type();
extern void		update_hp_offset();
extern void		update_hp_size();
extern void		update_hp_size_slider();
extern void		update_hp_image();

/* screens (start at 1) */

#define MACHINE_SCREEN 1
#define DEFAULTS_SCREEN 2
#define CLIENT_SCREEN 3
#define SOFTWARE_SCREEN 4
#define DISK_SCREEN 5

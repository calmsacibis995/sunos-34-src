
#ifndef lint
static	char sccsid[] = "@(#)image.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup.h"

#define	TEXT_FONT		"/usr/lib/fonts/fixedwidthfonts/screen.r.12"
#define	IMAGE_FONT		"/usr/lib/fonts/fixedwidthfonts/screen.b.14"
#define	IMAGE_LABEL_FONT	"/usr/lib/fonts/fixedwidthfonts/screen.b.12"

/* Fonts used for image and panel text strings */
Pixfont		*text_font;
Pixfont		*image_font;
Pixfont		*image_label_font;

/* Common button images */
Pixrect		*image_modify;
Pixrect		*image_add;
Pixrect		*image_delete;
Pixrect		*image_apply;
Pixrect		*image_close;
Pixrect		*image_status;
Pixrect		*image_help;
Pixrect		*image_done;
Pixrect		*image_exit;
Pixrect		*image_reboot;
Pixrect		*image_execute;
Pixrect		*image_machine;
Pixrect		*image_disks;
Pixrect		*image_clients;
Pixrect		*image_soft_parts;
Pixrect		*image_config_cards;
Pixrect		*image_software;
Pixrect		*image_defaults;
Pixrect		*image_ok;
Pixrect		*image_no;
Pixrect		*image_yes;

/* Common iconic images */
static short    disk_dat[] = {
#include "images/wcdisk.icon"
};
mpr_static(image_disk_pr, 64, 54, 1, disk_dat);

static short    controller1_dat[] = {
#include "images/controller1.icon"
};
mpr_static(image_controller1_pr, 64, 64, 1, controller1_dat);

static short    controller2_dat[] = {
#include "images/controller2.icon"
};
mpr_static(image_controller2_pr, 64, 64, 1, controller2_dat);

static short    xy_disk_dat[] = {
#include "images/xy_disk.icon"
};
mpr_static(image_xy_disk_pr, 64, 54, 1, xy_disk_dat);

static short    xy_controller1_dat[] = {
#include "images/xy_controller1.icon"
};
mpr_static(image_xy_controller1_pr, 64, 64, 1, xy_controller1_dat);

static short    xy_controller2_dat[] = {
#include "images/xy_controller2.icon"
};
mpr_static(image_xy_controller2_pr, 64, 64, 1, xy_controller2_dat);

static short    sd_disk_dat[] = {
#include "images/sd_disk.icon"
};
mpr_static(image_sd_disk_pr, 64, 54, 1, sd_disk_dat);

static short    sd_controller1_dat[] = {
#include "images/sd_controller1.icon"
};
mpr_static(image_sd_controller1_pr, 64, 64, 1, sd_controller1_dat);

static short    sd_controller2_dat[] = {
#include "images/sd_controller2.icon"
};
mpr_static(image_sd_controller2_pr, 64, 64, 1, sd_controller2_dat);

#ifdef NOTDEF
    static short    hardpart_pr_dat[] = {
    #include "images/hardpart.pr"
    };
    mpr_static(image_hardpart_pr, 64, 18, 1, hardpart_pr_dat);
#endif NOTDEF

static short    partition_pr_dat[] = {
#include "images/partition.pr"
};
mpr_static(image_partition_pr, 48, 18, 1, partition_pr_dat);

static short    client_pr_dat[] = {
#include "images/client.icon"
};
mpr_static(image_client_pr, 64, 64, 1, client_pr_dat);

static short    card_pr_dat[] = {
#include "images/card.icon"
};
mpr_static(image_card_pr, 64, 64, 1, card_pr_dat);

static short    cycle_dat[] = {
#include "images/cycle.pr"
};
mpr_static(cycle_pr, 16, 16, 1, cycle_dat);

static unsigned short	grey_25_data[16] = {
			0x8888, 0x2222, 0x4444, 0x1111,
			0x8888, 0x2222, 0x4444, 0x1111,
			0x8888, 0x2222, 0x4444, 0x1111,
			0x8888, 0x2222, 0x4444, 0x1111
		};
mpr_static(image_grey_25_pr, 16, 16, 1, grey_25_data);


Pixrect		*image_disk = &image_disk_pr;
Pixrect		*image_controller1 = &image_controller1_pr;
Pixrect		*image_controller2 = &image_controller2_pr;
Pixrect		*image_xy_disk = &image_xy_disk_pr;
Pixrect		*image_xy_controller1 = &image_xy_controller1_pr;
Pixrect		*image_xy_controller2 = &image_xy_controller2_pr;
Pixrect		*image_sd_disk = &image_sd_disk_pr;
Pixrect		*image_sd_controller1 = &image_sd_controller1_pr;
Pixrect		*image_sd_controller2 = &image_sd_controller2_pr;
Pixrect		*image_hardpart = &image_partition_pr;
Pixrect		*image_softpart = &image_partition_pr;
Pixrect		*image_client = &image_client_pr;
Pixrect		*image_card = &image_card_pr;
Pixrect         *image_cycle = &cycle_pr;
Pixrect		*image_grey_25 = &image_grey_25_pr;
 
/* Initialize the global images */
void
image_init(panel)
    Panel	panel;
{
    text_font = pf_open(TEXT_FONT);
    if (!text_font) {
        setup_error("image_init: can't open font file %s\n", TEXT_FONT);
        return;
    }
    image_font = pf_open(IMAGE_FONT);
    if (!image_font) {
        setup_error("image_init: can't open font file %s\n", IMAGE_FONT);
        return;
    }
    image_label_font = pf_open(IMAGE_LABEL_FONT);
    if (!image_label_font) {
        setup_error("image_init: can't open font file %s\n", IMAGE_LABEL_FONT);
        return;
    }
       
    image_modify	= panel_button_image(panel, "Modify", 0, image_font);
    image_add		= panel_button_image(panel, "Add", 0, image_font);
    image_delete	= panel_button_image(panel, "Delete", 0, image_font);
    image_apply		= panel_button_image(panel, "Apply", 0, image_font);
    image_close		= panel_button_image(panel, "Close", 0, image_font);
    image_status	= panel_button_image(panel, "Status", 0, image_font);
    image_help		= panel_button_image(panel, "Help", 0, image_font);
    image_done		= panel_button_image(panel, "Done", 0, image_font);
    image_exit		= panel_button_image(panel, "Quit", 0, image_font);
    image_execute	= panel_button_image(panel, 
    					     "Execute-Setup", 0, image_font);
    image_reboot	= panel_button_image(panel, "Reboot", 0, image_font);
    image_machine	= panel_button_image(panel, 
                                             "Workstation", 0, image_font);
    image_disks		= panel_button_image(panel, "Disks", 0, image_font);
    image_clients	= panel_button_image(panel, "Clients", 0, image_font);
    image_soft_parts	= panel_button_image(panel, 
                                             "Soft Partitions", 0, image_font);
    image_config_cards	= panel_button_image(panel,  
                                             "Config Cards", 0, image_font);
    image_software	= panel_button_image(panel, "Software", 0, image_font);
    image_defaults	= panel_button_image(panel, "Defaults", 0, image_font);

    image_ok		= panel_button_image(panel, "Ok", 0, image_font);
    image_no		= panel_button_image(panel, "No", 0, image_font);
    image_yes		= panel_button_image(panel, "Yes", 0, image_font);
    
    if (image_modify && image_add && image_delete && image_close &&
            image_status && image_help && image_done && image_machine && 
            image_disks && image_clients && image_soft_parts && 
            image_config_cards && image_software && 
	    image_ok && image_no && image_yes)
        return;
        
    setup_error("image_init: can't create button images\n");
}

/* image_label_pr writes the label 'string' at the bottom of 'src_pr'    */
/* if 'position' is non-negative, it left justifies the label 'position' */ 
/* pixels from the left. it centers the label otherwise. long labels are */
/* clipped                                                               */
Pixrect *
image_label_pr(src_pr, string, position)
    Pixrect	*src_pr;
    char	*string;
    int		position;
{
    struct pr_prpos  where;	/* where to write the label */
    struct pr_size   label_size;	/* size of the label */

				/* copy the source pixrect */
    where.pr = mem_create(src_pr->pr_size.x, src_pr->pr_size.y, 1);
    pr_rop(where.pr, 0, 0, src_pr->pr_size.x, src_pr->pr_size.y, 
	   PIX_SRC, src_pr, 0, 0);
				/* choose a font which fits ? */
    label_size = pf_textwidth(strlen(string), image_label_font, string);
    if (label_size.x > src_pr->pr_size.x) 
        label_size.x = src_pr->pr_size.x;
				/* position and add the label */
    if (position < 0)  		/* center the label */
        if (label_size.x > src_pr->pr_size.x) 
            where.pos.x = 0;
        else 
	    where.pos.x = (src_pr->pr_size.x - label_size.x) / 2;
    else  			/* left justifiy it */
	if (position > src_pr->pr_size.x)
            return (where.pr);
        else 
	    where.pos.x = position;
    where.pos.y = src_pr->pr_size.y - 7;
    pf_text(where, PIX_SRC, image_label_font, string);
   
    return (where.pr);
}


/* re-label the pixrect 'src_pr' with 'string'
 * center the label if position is < 0  
 */ 
void
image_relabel_pr(src_pr, string, position)
    Pixrect	*src_pr;
    char	*string;
    int		position;
{
    struct pr_prpos  where;		/* where to write the label */
    struct pr_size   label_size;	/* size of the label */

    where.pr = src_pr;
    label_size = pf_textwidth(strlen(string), image_label_font, string);
    if (label_size.x > src_pr->pr_size.x) 
        label_size.x = src_pr->pr_size.x;
					/* position and add the label */
    if (position < 0)  			/* center the label */
        if (label_size.x > src_pr->pr_size.x) 
            where.pos.x = 0;
        else 
	    where.pos.x = (src_pr->pr_size.x - label_size.x) / 2;
    else  				/* left justifiy it */
	if (position > src_pr->pr_size.x)
            return;
        else 
	    where.pos.x = position;
    where.pos.y = src_pr->pr_size.y - 7;
    					/* overwrite the old label */
    pr_rop(where.pr, 0, where.pr->pr_height - 18, where.pr->pr_width, 18,
               PIX_CLR, 0, 0, 0);
    pf_text(where, PIX_SRC, image_label_font, string);
}


/* create a pixrect with a string in it.
 * If width is > 0 then make it width pixels wide
 * and center the string.
 */
Pixrect *
image_string_width(string, width)
char	*string;
int	width;
{
    struct pr_prpos  where;	/* where to write the string */
    struct pr_size   size;	/* size of the string */

    size = pf_textwidth(strlen(string), image_label_font, string);
    where.pr = mem_create( width > 0 ? width : size.x + 8, size.y + 8, 1);
    if (!where.pr)
        return (0);

    where.pos.x = (where.pr->pr_width - size.x) / 2;
    where.pos.y = 4 - (image_label_font->pf_char['n'].pc_home.y);
    pf_text(where, PIX_SRC, image_label_font, string);
    
    return (where.pr);
}


/* draw a box around pixrect pr */
void
image_box(pr, op)
register Pixrect	*pr;
register int		op;
{
    register	x_right	= pr->pr_width - 1;
    register	y_bot	= pr->pr_height - 1;

    pr_vector(pr, 0, 0, x_right, 0, op, 1);
    pr_vector(pr, x_right, 0, x_right, y_bot, op, 1);
    pr_vector(pr, x_right, y_bot, 0, y_bot, op, 1);
    pr_vector(pr, 0, y_bot, 0, 0, op, 1);
}


/* invert a pixrect */
void
image_invert(pr)
struct pixrect	*pr;
{
    pr_rop(pr, 0, 0, pr->pr_width, pr->pr_height, PIX_NOT(PIX_DST),
	0, 0, 0);
}


/* grey a pixrect */
void
image_grey(pr)
struct pixrect	*pr;
{
    pr_replrop(pr, 0, 0, pr->pr_width, pr->pr_height, PIX_SRC | PIX_DST,
	image_grey_25, 0, 0);
}

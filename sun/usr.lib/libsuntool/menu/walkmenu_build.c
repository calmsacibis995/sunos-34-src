#ifndef lint
static	char sccsid[] = "@(#)walkmenu_build.c 1.1 86/09/25 Copyright 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*-
	WALKING MENU PACKAGE

	walkmenu_build.c, Sun Jun 30 15:38:39 1985

		Craig Taylor,
		Sun Microsystems
 */

#include <sys/types.h>
#include <stdio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>

#include "walkmenu_impl.h"

#define	MENU_DEFAULT_MARGIN		1
#define	MENU_DEFAULT_LEFT_MARGIN	16
#define	MENU_DEFAULT_RIGHT_MARGIN	6

#define imax(a, b) ((int)(b) > (int)(a) ? (int)(b) : (int)(a))

#define MAX_ITEM 100

static	short arrow_data[] = {
	0x0200, 0x0300, 0x7E80, 0x00C0, 0x7E80, 0x0300, 0x0200, 0x0000
};
static	mpr_static(arrow_pr, 12, 8, 1, arrow_data);

static	short gray25_data[16] = { /* 25% gray so background will show through */
	0x8888, 0x2222, 0x4444, 0x1111, 0x8888, 0x2222, 0x4444, 0x1111,
	0x8888, 0x2222, 0x4444, 0x1111, 0x8888, 0x2222, 0x4444, 0x1111
};
	mpr_static(menu_gray25_pr, 16, 16, 1, gray25_data);

static	short gray50_data[16] = { /* 50% gray */
	0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,
	0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555
};
	mpr_static(menu_gray50_pr, 16, 16, 1, gray50_data);

static	short gray75_data[16] = { /* 75% gray */
	0x7777,0xDDDD,0xBBBB,0xEEEE,0x7777,0xDDDD,0xBBBB,0xEEEE,
	0x7777,0xDDDD,0xBBBB,0xEEEE,0x7777,0xDDDD,0xBBBB,0xEEEE
};
	mpr_static(menu_gray75_pr, 16, 16, 1, gray75_data);

struct menu_ops menu_ops = { menu_gets, menu_sets, menu_destroys};
struct menu_ops menu_item_ops =
    { menu_item_gets, menu_item_sets, menu_item_destroys};

#ifdef not
/* VARARGS */
Menu
menu_build(first_attr)
	caddr_t	*first_attr;
{   
    Menu_attribute avlist[ATTR_STANDARD_SIZE];
    attr_make(avlist, ATTR_STANDARD_SIZE, &first_attr);
    return menu_create(ATTR_LIST, avlist, 0);
}
#endif

/* VARARGS */
Menu
menu_create(first_attr)
	caddr_t	*first_attr;
{   
    Menu_attribute avlist[ATTR_STANDARD_SIZE];
    register struct menu *m = (struct menu *)calloc(sizeof(struct menu), 1);
    
    m->default_image.margin = MENU_DEFAULT_MARGIN;
    m->default_image.left_margin = MENU_DEFAULT_LEFT_MARGIN;
    m->default_image.right_margin = MENU_DEFAULT_RIGHT_MARGIN;
    m->ops = &menu_ops;
    m->nitems = 0, m->max_nitems = 2 * MENU_FILLER;
    m->item_list = (struct menu_item **)
	malloc(sizeof(struct menu_item *) * 2 * MENU_FILLER);
    if (!m->item_list) {
	fprintf(stderr,
		"menu_create: Malloc failed to allocate an item list.\n");
	return NULL;
    }

    attr_make(avlist, ATTR_STANDARD_SIZE, &first_attr);
    menu_sets(m, avlist);

    if (!m->shadow_pr) m->shadow_pr = MENU_DEFAULT_SHADOW;
    if (!m->notify_proc) m->notify_proc = MENU_DEFAULT_NOTIFY_PROC;
    if (!m->accelerated_selection) m->accelerated_selection = MENU_DEFAULT_ITEM;
    if (!m->default_selection) m->default_selection = MENU_DEFAULT_ITEM;

#					ifdef not
    while (next_item-- > 0) {	
	item = il[next_item] = item_list[next_item];
	std_image->width = (int)image_get(item->image, std_image, IMAGE_WIDTH);
	std_image->height = (int)image_get(item->image, std_image, IMAGE_HEIGHT);
    }
    m->item_list = il;
#					endif not
    return (Menu)m;
}


static
struct menu_item *
menu_create_item_avlist(avlist)
	Menu_attribute *avlist;
{   
    struct menu_item *mi;

    mi = (struct menu_item *)calloc(sizeof(struct menu_item), 1);
    mi->image = (struct image *)image_create(IMAGE_RELEASE, 0);
    mi->ops = &menu_item_ops;

    menu_item_sets(mi, avlist);

    return mi;
}


/* VARARGS */
Menu_item
menu_create_item(first_attr)
	caddr_t	*first_attr;
{
    Menu_attribute avlist[ATTR_STANDARD_SIZE];

    attr_make(avlist, ATTR_STANDARD_SIZE, &first_attr);
    return (Menu_item)menu_create_item_avlist(avlist);
}


/* VARARGS1 */
int
menu_set(m, first_attr)
	struct menu *m;
	Menu_attribute first_attr;
{
    Menu_attribute avlist[ATTR_STANDARD_SIZE];

    attr_make(avlist, ATTR_STANDARD_SIZE, &first_attr);
    return m ? (((struct menu *)m)->ops->menu_set_op)(m, avlist) : FALSE;
}


static int
menu_sets(m, attrs)
	register struct menu *m;
	register Menu_attribute *attrs;
{   
    for (; *attrs; attrs = menu_attr_next(attrs))
	switch (attrs[0]) {

	  case MENU_ACCELERATED_SELECTION:
	    m->accelerated_selection = (Menu_attribute)attrs[1];
	    if (!m->accelerated_selection)
		m->accelerated_selection = MENU_SELECTED_ITEM;
	    break;
	    
	  case MENU_ACTION_IMAGE:
	  case MENU_ACTION_ITEM:
	  case MENU_GEN_PROC_IMAGE:
	  case MENU_GEN_PROC_ITEM:
	  case MENU_IMAGE_ITEM:
	  case MENU_PULLRIGHT_IMAGE:
	  case MENU_PULLRIGHT_ITEM:
	  case MENU_STRING_ITEM:
	    if (m->nitems < m->max_nitems || extend_item_list(m)) {
		m->item_list[m->nitems++] = (struct menu_item *)
		    menu_create_item(MENU_RELEASE,
				     attrs[0], attrs[1], attrs[2], 0);
	    }
	    break;

	  case MENU_APPEND_ITEM:
	    if (m->nitems < m->max_nitems || extend_item_list(m))
		m->item_list[m->nitems++] = (struct menu_item *)attrs[1];
	    break;
	    
	  case MENU_CLIENT_DATA:
	    m->client_data = (caddr_t)attrs[1];
	    break;
	    
	  case MENU_DEFAULT:
	    m->default_item = (int)attrs[1];
	    break;
	    
	  case MENU_DEFAULT_ITEM:
	    m->default_item = lookup(m->item_list, m->nitems, attrs[1]);
	    break;
	    
	  case MENU_DEFAULT_IMAGE:
	    if (attrs[1]) m->default_image = *(struct image *)attrs[1];
	    break;
	    
	  case MENU_DEFAULT_SELECTION:
	    m->default_selection = (Menu_attribute)attrs[1];
	    break;
	    
	  case MENU_DISPLAY_ONE_LEVEL:
	    m->display_one_level = (int)attrs[1];
	    break;
	    
	  case MENU_FONT:
	    m->default_image.font = (struct pixfont *)attrs[1];
	    break;
	    
	  case MENU_GEN_PROC:
	    m->gen_proc = (struct menu *(*)())attrs[1];
	    break;

	  case MENU_HEIGHT:
	    m->height = (int)attrs[1];
	    break;
	    
	  case MENU_IMAGES:
	    {   
		char **a = (char **)&attrs[1];
		while (*a) {
		    if (m->nitems < m->max_nitems || extend_item_list(m)) {
			m->item_list[m->nitems] =
			    (struct menu_item *)
				menu_create_item(MENU_RELEASE, MENU_IMAGE_ITEM,
						 *a++, m->nitems + 1, 0);
		    }
		    m->nitems++;
		}
	    }
	    break;
	    
	  case MENU_INSERT:
	    if (m->nitems < m->max_nitems || extend_item_list(m)) {
		insert_item(m->item_list, m->max_nitems, m->nitems,
			    (int)attrs[1], (Menu_item)attrs[2]);
	    }
	    break;
	    
	  case MENU_INSERT_ITEM:
	    if (m->nitems < m->max_nitems || extend_item_list(m)) {
		insert_item(m->item_list, m->max_nitems, m->nitems,
			    lookup(m->item_list, m->nitems, attrs[1]),
			    (Menu_item)attrs[2]);
	    }
	    break;
	    
	  case MENU_ITEM:
	    if (m->nitems < m->max_nitems || extend_item_list(m)) {
		m->item_list[m->nitems] =
		    (struct menu_item *)menu_create_item_avlist(&attrs[1]);
	    }
	    menu_set(m->item_list[m->nitems++], MENU_RELEASE, 0);
	    break;
	    
	  case MENU_LEFT_MARGIN:
	    m->default_image.left_margin = (int)attrs[1];
	    break;
	    
/*        case /--/MENU_LIKE: Copy menu and items into current menu
	    break;
 */	    
	  case MENU_MARGIN:
	    m->default_image.margin = (int)attrs[1];
	    break;
	    
	  case MENU_NCOLS:
	    m->ncols = imax(0, (int)attrs[1]);
	    break;

	  case MENU_NROWS:
	    m->nrows = imax(0, (int)attrs[1]);
	    break;

	  case MENU_NOTIFY_PROC:
	    m->notify_proc = (caddr_t (*)())attrs[1];
	    if (!m->notify_proc) m->notify_proc = MENU_DEFAULT_NOTIFY_PROC;
	    break;
	    
	  case MENU_REMOVE:
	    if (remove_item(m->item_list, m->nitems, (int)attrs[1]))
		--m->nitems;
	    break;
	    
	  case MENU_REMOVE_ITEM:
	    if (remove_item(m->item_list, m->nitems, 
			    lookup(m->item_list, m->nitems, attrs[1])))
	    --m->nitems;
	    break;
	    
	  case MENU_REPLACE:
	    replace_item(m->item_list, m->nitems, (int)attrs[1],
			 (Menu_item)attrs[2]);
	    break;
	    
	  case MENU_REPLACE_ITEM:
	    replace_item(m->item_list, m->nitems,
			 lookup(m->item_list, m->nitems, attrs[1]),
			 (Menu_item)attrs[2]);
	    break;
	    
	  case MENU_RIGHT_MARGIN:
	    m->default_image.right_margin = (int)attrs[1];
	    break;
	    
	  case MENU_SELECTED:
	    m->selected_item = (int)attrs[1];
	    break;
	    
	  case MENU_SELECTED_ITEM:
	    m->selected_item = lookup(m->item_list,m->nitems, attrs[1]);
	    break;
	    
	  case MENU_SHADOW:
	    m->shadow_pr = (struct pixrect *)attrs[1];
	    break;
	    
	  case MENU_STRINGS:
	    {   
		char **a = (char **)&attrs[1];
		while (*a) {
		    if (m->nitems < m->max_nitems || extend_item_list(m)) {
			m->item_list[m->nitems] =
			    (struct menu_item *)
				menu_create_item(MENU_RELEASE, MENU_STRING_ITEM,
						 *a++, m->nitems + 1, 0);
		    }
		    m->nitems++;
		}
	    }
	    break;
	    
	  case MENU_TITLE_ITEM:
	  case MENU_TITLE_IMAGE:
	    if (m->nitems < m->max_nitems || extend_item_list(m))
		m->item_list[m->nitems++] = (struct menu_item *)
		    menu_create_item(MENU_TITLE_ITEM == attrs[0]
				     ? MENU_STRING : MENU_IMAGE, attrs[1], 
				     MENU_INVERT, TRUE, MENU_FEEDBACK, FALSE,
				     MENU_RELEASE,
				     MENU_NOTIFY_PROC, menu_return_no_value,
				     0);
	    break;

	  case MENU_WIDTH:
	    m->width = (int)attrs[1];
	    break;
	    
	  case MENU_NOP:
	    break;
	
	  default:
	    if (ATTR_PKG_MENU == ATTR_PKG(attrs[0]))
		fprintf(stderr,
			"menu_set:  Menu attribute not allowed.\n%s\n",
			attr_sprint(NULL, attrs[0]));
		break;
	    
	}
}


static int
menu_item_sets(mi, attrs)
	register struct menu_item *mi;
 	register Menu_attribute *attrs;
{   
    int invert = FALSE;
    
    for (; *attrs; attrs = menu_attr_next(attrs))
	switch (attrs[0]) {

	  case MENU_ACTION:
	    mi->notify_proc = (caddr_t (*)())attrs[1];
	    break;

	  case MENU_ACTION_ITEM:
	    if (mi->image) {
		mi->image->string = (char *)attrs[1];
		mi->notify_proc = (caddr_t (*)())attrs[2];
	    }
	    break;

	  case MENU_STRING_ITEM:
	    if (mi->image) {
		mi->image->string = (char *)attrs[1];
		mi->value = (int)attrs[2];
	    }
	    break;

	  case MENU_ACTION_IMAGE:
	    if (IMAGE_P(attrs[1])) {
		image_destroy(mi->image);
		mi->image = (struct image *)attrs[1];
	    } else if (mi->image) {
		mi->image->pr = (struct pixrect *)attrs[1];
	    }
	    mi->notify_proc = (caddr_t (*)())attrs[2];
	    break;

	  case MENU_IMAGE_ITEM:
	    if (IMAGE_P(attrs[1])) {
		image_destroy(mi->image);
		mi->image = (struct image *)attrs[1];
	    } else if (mi->image) {
		mi->image->pr = (struct pixrect *)attrs[1];
	    }
	    mi->value = (int)attrs[2];
	    break;

	  case MENU_CLIENT_DATA:
	    mi->client_data = (caddr_t)attrs[1];
	    break;
	    
	  case MENU_FEEDBACK:
	    mi->no_feedback = !(int)attrs[1];
	    break;
	    
	  case MENU_FONT:
	    if (mi->image) mi->image->font = (struct pixfont *)attrs[1];
	    break;

	  case MENU_GEN_PROC:
	    mi->gen_proc = (struct menu *(*)())attrs[1];
	    mi->pullright = mi->gen_proc != NULL;
	    image_set(mi->image, IMAGE_RIGHT_PIXRECT,
		      mi->pullright ? &arrow_pr : NULL);
	    break;

	  case MENU_GEN_PROC_IMAGE:
	    if (IMAGE_P(attrs[1])) {
		image_destroy(mi->image);
		mi->image = (struct image *)attrs[1];
	    } else {
		image_set(IMAGE_PIXRECT, attrs[1]);
		image_set(IMAGE_RIGHT_PIXRECT, &arrow_pr);
	    }
	    mi->gen_proc = (struct menu *(*)())attrs[2];
	    mi->pullright = mi->gen_proc != NULL;
	    mi->value = 0;
	    break;

	  case MENU_GEN_PROC_ITEM:
	    if (mi->image) {
		mi->image->string = (char *)attrs[1];
		mi->image->right_pr = &arrow_pr;
	    }
	    mi->gen_proc = (struct menu *(*)())attrs[2];
	    mi->pullright = mi->gen_proc != NULL;
	    mi->value = 0;
	    break;
	    
	  case MENU_IMAGE:
	    if (mi->image) mi->image->pr = (struct pixrect *)attrs[1];
	    break;

	  case MENU_INACTIVE:
	    mi->inactive = (int)attrs[1];
	    break;

	  case MENU_INVERT:
	    invert = (int)attrs[1];
	    break;
	    
	  case MENU_LEFT_MARGIN:
	    if (mi->image) mi->image->left_margin = (int)attrs[1];
	    break;
	    
	  case MENU_MARGIN:
	    if (mi->image) mi->image->margin = (int)attrs[1];
	    break;
	    
	  case MENU_NOTIFY_PROC:
	    mi->notify_proc = (caddr_t (*)())attrs[1];
	    break;
	    
	  case MENU_PULLRIGHT:
	    mi->value = (int)attrs[1];
	    mi->pullright = mi->value != NULL;
	    image_set(mi->image, IMAGE_RIGHT_PIXRECT,
		      mi->pullright ? &arrow_pr : NULL);
	    break;

	  case MENU_PULLRIGHT_IMAGE:
	    if (IMAGE_P(attrs[1])) {
		image_destroy(mi->image);
		mi->image = (struct image *)attrs[1];
	    } else if (mi->image) {
		    mi->image->pr = (struct pixrect *)attrs[1];
		    mi->image->right_pr = &arrow_pr;
	    }
	    mi->value = (int)attrs[2];
	    mi->pullright = mi->value != NULL;
	    break;

	  case MENU_PULLRIGHT_ITEM:
	    if (mi->image) {
		mi->image->string = (char *)attrs[1];
		mi->image->right_pr = &arrow_pr;
	    }
	    mi->value = (int)attrs[2];
	    mi->pullright = mi->value != NULL;
	    break;

	  case MENU_RELEASE:
	    mi->free_item = TRUE;
	    break;
	    
	  case MENU_RIGHT_MARGIN:
	    if (mi->image) mi->image->right_margin = (int)attrs[1];
	    break;
	    
	  case MENU_STRING:
	    if (mi->image) mi->image->string = (char *)attrs[1];
	    break;
	    
	  case MENU_VALUE:
	    mi->value = (int)attrs[1];
	    mi->pullright = FALSE;
	    image_set(mi->image, IMAGE_RIGHT_PIXRECT, NULL);
	    break;
	    
	  case MENU_NOP:
	    break;
	
	  default:
	    if (ATTR_PKG_MENU == ATTR_PKG(attrs[0]))
		fprintf(stderr,
			"menu_set(item):  Menu attribute not allowed.\n%s\n",
			attr_sprint(NULL, attrs[0]));
		break;

	}
    if (invert && mi->image) mi->image->invert = TRUE;
}


static int
extend_item_list(m)
	register struct menu *m;
{   
    m->max_nitems =  m->max_nitems + MENU_FILLER;
    m->item_list = (struct menu_item **)realloc(m->item_list, m->max_nitems *
						sizeof(struct menu_item *));
    if (!m->item_list) {
	fprintf(stderr, "menu_set: Malloc failed to allocate an item list.\n");
	perror("menu_set");
	m->max_nitems =  m->max_nitems - MENU_FILLER;
	return FALSE;
    }
    return TRUE;
}


static int
remove_item(il, len, pos)
	register struct menu_item *il[];
{
    register int i;
    if (pos < 1 || pos > len) return FALSE;
    for (i = pos; i < len; i++) il[i - 1] = il[i];
    return TRUE;
}


static int
replace_item(il, len, pos, mi)
	struct menu_item *il[], *mi;
{
    if (pos < 1 || pos > len) return FALSE;
    il[pos - 1] = mi;
    return TRUE;
}


static int
insert_item(il, maxlen, len, pos, mi)
	register struct menu_item *il[];
	struct menu_item *mi;
{
    register int i;
    if (pos < 1 || pos >= len || maxlen <= len) return FALSE;
    for (i = len; i > pos; --i) il[i] = il[i - 1];
    il[pos - 1] = mi;
    return TRUE;
}


static int
lookup(il, len, mi)
	register struct menu_item *il[];
	struct menu_item *mi;
{
    int i;
    
    for (i = 0; i < len; i++) if (il[i] == mi) return i + 1;
    return 0;
}

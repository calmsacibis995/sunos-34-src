#ifndef lint
static	char sccsid[] = "@(#)walkmenu.c 1.1 86/09/25 Copyright 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*-
	WALKING MENU PACKAGE

	walkmenu.c, Sun Jun 30 15:38:39 1985

		Craig Taylor,
		Sun Microsystems
 */

#include <sys/types.h>
#include <stdio.h>

#include <sys/time.h>
#include <sunwindow/win_input.h>

#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>

#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>

#include <sunwindow/win_cursor.h>
#include <sunwindow/win_struct.h>
#include <suntool/fullscreen.h>
#include <fcntl.h>

#include <suntool/window.h>
		    
#include "walkmenu_impl.h"

extern struct pixrect arrow_pr;

#define	rect_expand(r)	(r)->r_left, (r)->r_top, (r)->r_width, (r)->r_height

#define	pwr_write(pw, r, op, pr, x, y) \
	pw_write((pw), \
	(r)->r_left, (r)->r_top, (r)->r_width, (r)->r_height, \
	(op), (pr), (x), (y))
#define	pwr_writebackground(pw, r, op) \
	pw_writebackground((pw), \
	(r)->r_left, (r)->r_top, (r)->r_width, (r)->r_height, \
	(op))

#define imax(a, b) ((int)(b) > (int)(a) ? (int)(b) : (int)(a))

static	struct fullscreen *m_fs;	/* the fullscreen object */
int heading;

/*
 * Cursor data
 */
static	short m_cursorimage[12] = {
	0x0000,	/*              	  */
	0x0020,	/*            # 	  */
	0x0030,	/*            ##	  */
	0x0038,	/*            ###	  */
       	0x003c,	/*            ####	  */
	0xfffe,	/*  ###############	  */
	0xffff,	/*  ################ X 	  */
	0xfffe,	/*  ###############	  */
       	0x003c,	/*            ####	  */
	0x0038,	/*            ###	  */
	0x0030,	/*            ##	  */
	0x0020	/*            # 	  */
};
static	mpr_static(m_mpr, 16, 12, 1, m_cursorimage);
static	struct	cursor mn_cursor = { 17, 6, PIX_SRC|PIX_DST, &m_mpr};

struct cursor *walkmenu_cursor = &mn_cursor;

#define	MENU_SHADOW	6
#define MENU_BORDER	1

static struct menu_item *last_selection;

#define MENU_PULLRIGHT_DELTA 15

/*
 * Display the menu, get the menu item, and call notify proc.
 * Default proc returns a pointer to the item selected or NULL.
 */
caddr_t
menu_show(menu, win, iep, first_attr)
	Menu menu;
	Window win;
	struct inputevent *iep;
	Menu_attribute *first_attr;
{   
    Menu_attribute avlist[ATTR_STANDARD_SIZE];
    Menu_attribute *attrs;
    int fd;
   
    fd = window_fd(win);
    if (first_attr) {
	attr_make(avlist, ATTR_STANDARD_SIZE, &first_attr);
	for (attrs = avlist; *attrs; attrs = menu_attr_next(attrs))
	    switch (attrs[0]) {

	      case MENU_BUTTON:
		iep->ie_code = (int)attrs[1];
		break;
	    
	      case MENU_FD: /* Remove this someday */
		fd = (int)attrs[1];
		break;
	    
	      case MENU_POS:
		iep->ie_locx = (int)attrs[1], iep->ie_locy = (int)attrs[2];
		break;
	    
	      case MENU_NOP:
		break;
	
	      default:
		if (ATTR_PKG_MENU == ATTR_PKG(attrs[0]))
		    fprintf(stderr,
			    "menu_show:  Menu attribute not allowed.\n%s\n",
			    attr_sprint(NULL, attrs[0]));
		    break;
	    }
    }
    menu_show_using_fd(menu, fd, iep);
}


menu_show_event(m, iep, fd)
	struct menu *m;
	struct inputevent *iep;
	int fd;
{   
    return (int)menu_show_using_fd(m, fd, iep);
}
    
caddr_t
menu_show_using_fd(m, fd, iep)
	struct menu *m;
	int fd;
	struct inputevent *iep;
{   
    struct inputevent ie;
    struct inputmask im;
   
   if (!m || m->nitems == 0) return (caddr_t)MENU_NO_VALUE;
   /*
    * Grab all input and disable anybody but windowfd from writing
    * to screen while we are violating window overlapping.
    */
    m_fs = fullscreen_init(fd);
    heading = 0;
   /*
    * Make sure input mask allows menu package to track cursor motion
    */
    im = m_fs->fs_cachedim;
    im.im_flags |= IM_NEGEVENT;
    win_setinputcodebit(&im, LOC_MOVEWHILEBUTDOWN);
    /* kludge cause can not put events back */
    win_setinputcodebit(&im, LOC_STILL);
    win_setinputcodebit(&im, iep->ie_code);
    win_setinputmask(m_fs->fs_windowfd, &im,
		     (struct inputmask *)NULL, WIN_NULLLINK);
    last_selection = (struct menu_item *)MENU_NO_ITEM;
    if (m->allow_accelerated_selection)
	accelerated_selection(m, m_fs->fs_windowfd, iep->ie_code);
    if (last_selection == (struct menu_item *)MENU_NO_ITEM) {
	ie = *iep;
	win_setcursor(m_fs->fs_windowfd, walkmenu_cursor);
	(void)show_menu(m, &ie, 0, 0);	/* Here we go */
    }
    fullscreen_destroy(m_fs);
   /*
    * Call notify_proc.  Should handle special case of selection = 0
    */
    if (last_selection) {
	win_setmouseposition(fd, iep->ie_locx, iep->ie_locy);
	if (last_selection->pullright && last_selection->value)
	    ((struct menu *)last_selection->value)->selected_item = 0;
    } else m->selected_item = 0;
    return (m->notify_proc)(m, last_selection);
}


/* 
 * Show_menu modifies the inputevent parameter iep.
 * It should contain the last active inputevent read for the fd.
 */
static int
show_menu(m, iep, subrect, reenter)
	register struct menu *m;
	register struct inputevent *iep;
	struct rectnode *subrect;
{   
    register int curitem, newitem;
    register max_width, max_height;
    register struct menu_item **item, *mi;
    struct rectnode menu_node;
    struct image *std_image;
    struct menu *(*gen_proc)();
    struct rect menurect, activerect, saverect;
    struct pixrect *oldbits, *pr, *image_pr;
    int stand_off = !reenter, status, top, left;
    int margin, right_margin;
    int ncols, nrows, i, j, n;
    register short menu_button = iep->ie_code;

    if (gen_proc = m->gen_proc) m = (gen_proc)(m, MENU_CREATE);
    if (!m) {
	fprintf(stderr, "menu_show: gen_proc failed to generate a menu.\n");
	return -1;
    }
    std_image = &m->default_image;
    margin = std_image->margin;
    right_margin = std_image->right_margin;
    /* Compute max size if any of the items have changed */
    max_width = std_image->width;
    max_height = std_image->height;
    for (item = m->item_list, curitem = 0, newitem = m->nitems;
	 mi = *item++, curitem < newitem; curitem++)
	if (!mi->image->width) {
	    max_width =
		imax(image_get(mi->image, std_image, IMAGE_WIDTH), max_width);
	    max_height =
		imax(image_get(mi->image, std_image, IMAGE_HEIGHT), max_height);
	}
    std_image->width = max_width;
    std_image->height = max_height;

    /*
     * Find the current menu item
     */
    if (reenter) {	/* Based on default selection */
	curitem = get_curitem(m, m->default_selection);
    } else {		/* Based on accelerated selection */
	curitem = get_curitem(m, m->accelerated_selection);
    }
    /*
     * Lay out menu space, adjusted for screen boundaries.
     */
    ncols = m->ncols;
    nrows = m->nrows;
    if (!(ncols && nrows)) {
	if (ncols) {    /*  case: ncols=n, nrows=to fit */
	    menurect.r_width = (ncols * max_width) +
		2*MENU_BORDER + (m->shadow_pr ? MENU_SHADOW : 0);
	    if (menurect.r_width > m_fs->fs_screenrect.r_width) {
		ncols = (m_fs->fs_screenrect.r_width
			- 2*MENU_BORDER + (m->shadow_pr ? MENU_SHADOW : 0))
		       / max_width;
	    }
	    nrows = (m->nitems - 1) / ncols + 1;
	} else { /* case: nrows=n, ncols=to fit */
	    if (!nrows) nrows = m->nitems;
	    menurect.r_height = (nrows * max_height) +
		2*MENU_BORDER + (m->shadow_pr ? MENU_SHADOW : 0);
	    if (menurect.r_height > m_fs->fs_screenrect.r_height) {
		nrows = (m_fs->fs_screenrect.r_height
			- 2*MENU_BORDER + (m->shadow_pr ? MENU_SHADOW : 0))
		       / max_height;
	    }
	    ncols = (m->nitems - 1) / nrows + 1;
	}
    }
    menurect.r_width = (ncols * max_width) +
	2*MENU_BORDER + (m->shadow_pr ? MENU_SHADOW : 0);
    menurect.r_height = (nrows * max_height) +
	2*MENU_BORDER + (m->shadow_pr ? MENU_SHADOW : 0);
    if (menurect.r_width > m_fs->fs_screenrect.r_width
	|| menurect.r_height > m_fs->fs_screenrect.r_height) {
	fprintf(stderr, "menu_show: Menu too large for screen.\n");
	return -1;
    }
    left = iep->ie_locx - ((curitem - 1) % ncols) * max_width;
    if (iep->ie_locx != left) stand_off = FALSE;
    menurect.r_left = left = left + (stand_off ? 3 : -3);

    menurect.r_top = top =
	iep->ie_locy - ((curitem - 1) / ncols) * max_height
	    - MENU_BORDER - max_height / 2;
    /*
     * Constrain to be on screen.
     */
    constrainrect(&menurect, &m_fs->fs_screenrect,
		  subrect ? &subrect->rn_rect : 0);
    saverect = menurect; /* Remember total size of affected area */
    if (m->shadow_pr) {
	menurect.r_width -= MENU_SHADOW;
	menurect.r_height -= MENU_SHADOW;
    }
    activerect = menurect;
    if (stand_off) {
	activerect.r_left -= 6;
	activerect.r_width += 6;
    }

    /*
     * Save current bits on screen.
     */
    oldbits = mem_create(saverect.r_width, saverect.r_height,
			 m_fs->fs_pixwin->pw_pixrect->pr_depth);
    if (oldbits == (struct pixrect *)NULL) {
	fprintf(stderr, "menu_show: Mem_create failed.\n");
	return -1;
    }	    
    pw_read(oldbits, 0, 0, saverect.r_width, saverect.r_height,
	    PIX_SRC|PIX_DONTCLIP,
	    m_fs->fs_pixwin, saverect.r_left, saverect.r_top);

    /*
     * Paint the menu.  For now assume that the menu is monochrome.
     */
    pr = mem_create(saverect.r_width, saverect.r_height, 1);
    image_pr = pr_region(pr, MENU_BORDER, MENU_BORDER,
			 max_width * ncols, max_height * nrows);

    paint_shadow(pr, m->shadow_pr, oldbits);

    if (left != menurect.r_left || top != menurect.r_top) {
	iep->ie_locx -= left - menurect.r_left;
	iep->ie_locy -= top - menurect.r_top;
	win_setmouseposition(m_fs->fs_windowfd, iep->ie_locx, iep->ie_locy);
    }

    top = n = 0;
    item = m->item_list;
    for (i = 0; i < nrows; i++) {
	left = 0;
	for (j = 0; j < ncols; j++) {
	    if (++n > m->nitems) break;
	    if ((*item)->inactive)
		image_render((*item)->image, std_image,
			     IMAGE_REGION, image_pr, left, top,
			     IMAGE_INACTIVE,
			     0);
	    else
		image_render((*item)->image, std_image,
			     IMAGE_REGION, image_pr, left, top,
			     0);
	    left += max_width;
	    item++;
	}
	top += max_height;
    }
    pw_lock(m_fs->fs_pixwin, &saverect);
    pw_preparesurface(m_fs->fs_pixwin, &saverect);
    pw_write(m_fs->fs_pixwin, saverect.r_left, saverect.r_top, 
	     pr->pr_width, pr->pr_height,
	     PIX_SRC, pr, 0, 0);
    if (!stand_off)
	feedback(m, &menurect, curitem, ncols, MENU_PROVIDE_FEEDBACK);
    pw_unlock(m_fs->fs_pixwin);

    if (!reenter && !m->display_one_level) {
	mi = m->item_list[curitem - 1];
	if (mi->pullright && !mi->inactive && (mi->value || mi->gen_proc)) {
	    int generate = mi->gen_proc != NULL;

	    if (generate) {
		mi->value = (int)mi->gen_proc(mi, MENU_CREATE);
		if (!mi->value) {
		    fprintf(stderr,
			    "menu_show: gen_proc failed to generate a menu.\n");
		    return -1;
		}
	    }
	    iep->ie_locx =
		(menurect.r_left + MENU_BORDER
		 + ((curitem - 1) % ncols + 1) * max_width
		 - right_margin - (arrow_pr.pr_width >> 1));
	    iep->ie_locy = 
		(menurect.r_top + MENU_BORDER
		 + (curitem - 1) / ncols * max_height
		 + (max_height >> 1));
	    menu_node.rn_next = subrect;
	    menu_node.rn_rect = menurect;
	    /* always set the event code back to the menu
	     * button we were given on entry.
	     */
	    if (stand_off)
		feedback(m, &menurect, curitem, ncols, MENU_PROVIDE_FEEDBACK);
	    event_set_id(iep, menu_button);
	    win_setmouseposition(m_fs->fs_windowfd,
				 iep->ie_locx, iep->ie_locy);
	    status =
		show_menu((struct menu *)mi->value, iep, &menu_node, reenter++);
	    if (generate) mi->gen_proc(mi, MENU_DESTROY);
	    if (status == 0 || --status != 0) goto exit;
	} else if (stand_off) curitem = 0;
    } else if (stand_off) curitem = 0;

/*     win_setmouseposition(m_fs->fs_windowfd, iep->ie_locx, iep->ie_locy);*/
   /*
    * Track the mouse.
    */
    for (;;) {
	register int itemx, itemy, locx;

	if (rect_includespoint(&activerect, iep->ie_locx, iep->ie_locy)) {
	   /*
	    * Check if cursor is in the current menu
	    */
	    itemx = iep->ie_locx - menurect.r_left - MENU_BORDER;
	    if (itemx < 0) itemx = m->nitems; /* Outside menu proper */
	    else {
		itemx = itemx / max_width;
		if (itemx >= ncols) itemx = ncols - 1;
	    }

	    itemy = (iep->ie_locy - menurect.r_top - MENU_BORDER) / max_height;
	    if (itemy < 0) itemy = 0;
	    else if (itemy >= nrows) itemy = nrows - 1;

	    newitem = itemy * ncols + itemx + 1;
	    if (newitem > m->nitems) newitem = 0;
	} else if (subrect) {
	   /*
	    * Check if cursor is in a parent menu
	    */
	    struct rectnode *rp;
	    for (rp = subrect, status = 1; rp; rp = rp->rn_next, status++) {
		if (rect_includespoint(&rp->rn_rect,
				       iep->ie_locx, iep->ie_locy))
		    goto exit;
	    }
	    newitem = 0;
	} else newitem = 0;
       /*
	* Provide feedback for new item.
	*/
	if (newitem != curitem) { /* revert item to normal state */
	    if (curitem)
		feedback(m, &menurect, curitem, ncols, MENU_REMOVE_FEEDBACK);
	    curitem = newitem;   /* invert new item */
	    if (curitem) {
		feedback(m, &menurect, curitem, ncols, MENU_PROVIDE_FEEDBACK);
		locx = iep->ie_locx;
	    }
	}
	if (newitem) {
	   /*
	    * If item is a menu, recurse.
	    */
	    mi = m->item_list[newitem - 1];
	    if (mi->pullright && !mi->inactive
		&& (iep->ie_locx > (menurect.r_left + MENU_BORDER
				    + (itemx + 1) * max_width
				    - right_margin - arrow_pr.pr_width)
		    || iep->ie_locx - locx > MENU_PULLRIGHT_DELTA)
		&& (mi->value || mi->gen_proc)) {
		int generate = mi->gen_proc != NULL;
		if (generate) {
		    mi->value = (int)mi->gen_proc(mi, MENU_CREATE);
		    if (!mi->value) {
			fprintf(stderr,
		"menu_show: gen_proc failed to generate a menu.\n");
			return -1;
		    }
		}
		iep->ie_code = iep->ie_code;
#ifdef not
		iep->ie_locx = (menurect.r_left + MENU_BORDER
				+ (itemx + 1) * max_width
				- right_margin - (arrow_pr.pr_width >> 1));
		iep->ie_locy = (menurect.r_top + MENU_BORDER
				+ itemy * max_height
				+ (max_height >> 1));
#endif not
		menu_node.rn_next = subrect;
		menu_node.rn_rect = menurect;

		/* always set the event code back to the menu
		 * button we were given on entry.
		 */
		event_set_id(iep, menu_button);

		status = show_menu((struct menu *)mi->value,
				   iep, &menu_node, ++reenter);
		locx = iep->ie_locx;
		if (generate) mi->gen_proc(mi, MENU_DESTROY);
		if (status == 0 || --status != 0) goto exit;
	    }
	}
       /*
	* Get next input event.
	*/
	do {
	    if (input_readevent(m_fs->fs_windowfd, iep) == -1) {
		fprintf(stderr, "menu_show: failed to track cursor.\n");
		perror("menu_show");
		status = -1;
		goto exit;
	    }
	   /*
	    * If button up is the menu button then
	    * an item has been selected,  return it.
	    */
	    if (menu_button == iep->ie_code && win_inputnegevent(iep)) {
		if (curitem) {
		    last_selection = m->item_list[curitem-1];
		    if (last_selection->inactive)
			last_selection = NULL, curitem = NULL;
		}
		status = 0;
		goto exit;
	    }
	} while (iep->ie_code != LOC_MOVEWHILEBUTDOWN &&
		 iep->ie_code != LOC_STILL);
    }

    /* Entry states at the next higher level:
     *   status<0 abort menu chain (error or null selection),
     *   status=0 valid selection save selected item,
     *   status>0 cursor has entered a parent menu.
     */

exit:
     pwr_write(m_fs->fs_pixwin, &saverect, PIX_SRC, oldbits, 0, 0);
     mem_destroy(oldbits);
     mem_destroy(pr);
     if (status == 0) {
	 m->selected_item = curitem;
     }
     if (gen_proc) (gen_proc)(m, MENU_DESTROY);
     return status;
}


int
menu_destroy(m)
	struct menu *m;
{
    return m ? (((struct menu *)m)->ops->menu_destroy_op)(m) : FALSE;
}


int
menu_destroys(m)
	register struct menu *m;
{   
    register int i;
    register struct menu_item *item;
    
    if (!m) return;
    for (i = 0; i < m->nitems; i++) {
	item = m->item_list[i];
	if (item->pullright && !item->gen_proc && item->value)
	    menu_destroy((struct menu *)item->value);
	if (item->free_item) menu_item_destroys(item);
    }
    free(m->item_list);
    free(m);
    return (int)MENU_NULL;
}


int
menu_item_destroys(mi)
	register struct menu_item *mi;
{
    if (!mi) return;
    if (mi->image->free_image) image_destroy(mi->image);
    free(mi);
    return (int)MENU_ITEM_NULL;
}


/*
 * ?? Needs more work.  Add small timeout for non-blocking read
 * 200msec?  Add attr for allow_accelerated_selection
 */
static
accelerated_selection(m, fd, code)
    register struct menu *m;
{
    struct inputevent ie;
    register struct inputevent *iep = &ie;
    register int dopt, item;
    
    last_selection = NULL;
    fcntl(fd, F_SETFL, FNDELAY);	/* Start non-blocking I/O */
    while (read(fd, iep, sizeof(ie)) != -1)
	if (iep->ie_code == code && win_inputnegevent(iep))
	    while (m) {
		item = get_curitem(m, m->accelerated_selection);
		last_selection = m->item_list[item - 1];
		if (last_selection->pullright && last_selection->value)
		    m = (struct menu *)last_selection->value; /* ?? generate? */
		else
		    m = NULL;
	    }
    dopt = fcntl(fd, F_GETFL, FNDELAY);/* Turn off non-blocking I/O */
    fcntl(fd, F_SETFL, dopt & ~FNDELAY);
}
    

#define range(v, min, max) ((v) >= (min) && (v) <= (max))

static int
get_curitem(m, v)
	struct menu *m;
	Menu_attribute v;
{
    switch (v) {

      case MENU_SELECTED_ITEM:
      case MENU_SELECTED:
	if (range(m->selected_item, 1, m->nitems))
	    return m->selected_item;
	else if (m->default_item && range(m->default_item, 1, m->nitems))
	    return m->default_item;
	else return 1;
	break;
	
      case MENU_DEFAULT_ITEM:
      case MENU_DEFAULT:
	if (range(m->default_item, 1, m->nitems))
	    return m->default_item;
	else return 1;
	break;
	
	break;
	
      case NULL:  /* handles the case when the behavior wasn''t specified */
      case MENU_NOP:
      default:
	if ((int)v > 0 && (int)v <= m->nitems) return (int)v;
	else return 1;
	break;

    }
}


menu_compute_size(m, colp, rowp)
	register struct menu *m;
{
/*  FIXME: to get screen size */
#ifdef not_implemented
    register int col = *colp, row = *rowp;
    if (!(cols && rows)) return;
    if (cols) {    /*  case: cols=n, rows=to fit */
	    if (!cols) cols = m->nitems;
	    menurect.r_width = (m->ncols * max_width) +
		2*MENU_BORDER + (m->shadow_pr ? MENU_SHADOW : 0);
	    if (menurect.r_width > m_fs->fs_screenrect.r_width) {
		cols = (m_fs->fs_screenrect.r_width
			- 2*MENU_BORDER + (m->shadow_pr ? MENU_SHADOW : 0))
		       / max_width;
	    }
	    rows = (m->nitems - 1) / cols + 1;
	} else { /* case: rows=n, cols=to fit */
	    menurect.r_height = (m->nrows * max_height) +
		2*MENU_BORDER + (m->shadow_pr ? MENU_SHADOW : 0);
	    if (menurect.r_height > m_fs->fs_screenrect.r_height) {
		rows = (m_fs->fs_screenrect.r_height
			- 2*MENU_BORDER + (m->shadow_pr ? MENU_SHADOW : 0))
		       / max_height;
	    }
	    cols = (m->nitems - 1) / rows + 1;
	}
    *colp = col, *rowp = row;
#endif
}

#ifdef debug
static console = 0;
#endif

static
paint_shadow(pr, shadow_pr, save_pr)
	register struct pixrect *pr, *shadow_pr, *save_pr;
{
    if (shadow_pr) {
 	/* Draw borders */
	pr_rop(pr, 0, 0, pr->pr_width - MENU_SHADOW, MENU_BORDER,
	       PIX_SET, 0, 0, 0);
	pr_rop(pr, 0, MENU_BORDER,
	       MENU_BORDER, pr->pr_height - MENU_SHADOW - 2*MENU_BORDER,
	       PIX_SET, 0, 0, 0);
	pr_rop(pr,
	       pr->pr_width - MENU_SHADOW - MENU_BORDER, MENU_BORDER,
	       MENU_BORDER, pr->pr_height - MENU_SHADOW - 2*MENU_BORDER,
	       PIX_SET, 0, 0, 0);
	pr_rop(pr, 0, pr->pr_height - MENU_SHADOW - MENU_BORDER,
	       pr->pr_width - MENU_SHADOW, MENU_BORDER,
	       PIX_SET, 0, 0, 0);
	/* Fill in with background */
	pr_rop(pr, pr->pr_width - MENU_SHADOW, 0,
		   MENU_SHADOW, pr->pr_height,
		   PIX_SRC, save_pr, pr->pr_width - MENU_SHADOW, 0);
	pr_rop(pr, 0, pr->pr_height - MENU_SHADOW,
		   pr->pr_width - MENU_SHADOW, MENU_SHADOW,
		   PIX_SRC, save_pr, 0, pr->pr_height - MENU_SHADOW);
	/* Rop shadow over background */
	pr_replrop(pr, pr->pr_width - MENU_SHADOW, MENU_SHADOW,
		   MENU_SHADOW, pr->pr_height - MENU_SHADOW,
		   PIX_SRC|PIX_DST, shadow_pr, 0/* pr->pr_width - MENU_SHADOW*/, 0);
	pr_replrop(pr, MENU_SHADOW, pr->pr_height - MENU_SHADOW,
		   pr->pr_width - 2 * MENU_SHADOW, MENU_SHADOW,
		   PIX_SRC|PIX_DST, shadow_pr, 0, 0/* pr->pr_height - MENU_SHADOW*/);
    } else {
 	/* Draw borders */
	pr_rop(pr, 0, 0, pr->pr_width, MENU_BORDER, PIX_SET, 0, 0, 0);
	pr_rop(pr, 0, MENU_BORDER - 1,
	       MENU_BORDER, pr->pr_height - 2*MENU_BORDER,
	       PIX_SET, 0, 0, 0);
	pr_rop(pr, pr->pr_width - MENU_BORDER - 1, MENU_BORDER - 1,
	       MENU_BORDER, pr->pr_height - 2*MENU_BORDER, PIX_SET, 0, 0, 0);
	pr_rop(pr, 0, pr->pr_height - MENU_BORDER - 1,
	       pr->pr_width, MENU_BORDER,
	       PIX_SET, 0, 0, 0);
    }
}


static
feedback(m, r, n, ncols, state)
	register struct menu *m;
	struct rect *r;
	Menu_feedback state;
{
    struct menu_item *mi = m->item_list[n - 1];
    int max_width = m->default_image.width;
    int max_height = m->default_image.height;
    int extended = FALSE;
    
    if (!mi->no_feedback && !m->feedback_proc && !mi->inactive) {
	int left, top;
	register int margin = m->default_image.margin;
	
	n--; /* Convert to zero origin */
	left = (r->r_left + MENU_BORDER + (extended ? 0 : margin)
		+ (n % ncols) * max_width);
	top = (r->r_top + MENU_BORDER + (extended ? 0 : margin)
	       + (n / ncols) * max_height);
	pw_write(m_fs->fs_pixwin, left, top,
		 max_width - (extended ? 0 : 2  * margin),
		 max_height - (extended ? 0 : 2 * margin),
		 PIX_NOT(PIX_DST), 0, 0, 0);
    } else if (m->feedback_proc)
	(m->feedback_proc)(mi, state); /* Fix?? Pass rect or region? */
}

static
constrainrect(rconstrain, rbound, rprev)
	register struct rect *rconstrain, *rbound, *rprev;
{
#ifdef later too fancy
    switch (heading) {

      case 0:
	if (rect_right(rconstrain) > rect_right(rbound) && rprev) {
	    rconstrain->r_top = rect_bottom(rprev) + MENU_SHADOW;
	    rconstrain->r_left = rprev->r_left;
	    heading = ++heading % 4;
	}
	break;

      case 1:
	rconstrain->r_top = rect_bottom(rprev) + MENU_SHADOW;
	rconstrain->r_left = rprev->r_left;
	if (rect_bottom(rconstrain) > rect_bottom(rbound)) {
	    rconstrain->r_left = rprev->r_left - rconstrain->r_width;
	    heading = ++heading % 4;
	}
	break;
	   
      case 2:
	rconstrain->r_top = rprev->r_top;
	rconstrain->r_left = rprev->r_left - rconstrain->r_width;
	if (rconstrain->r_left < rbound->r_left) {
	    rconstrain->r_top = rprev->r_top - rconstrain->r_height;
	    heading = ++heading % 4;
	}
	break;
	   
      case 3:
	rconstrain->r_top = rprev->r_top - rconstrain->r_height;
	rconstrain->r_left = rprev->r_left;
	if (rconstrain->r_top < rbound->r_top) {
	    rconstrain->r_left += rconstrain->r_width;
	    heading = ++heading % 4;
	}
	break;

    }
#endif
   /*
    * Bias algorithm to have too big rconstrain fall off right/bottom.
    */
    if (rect_right(rconstrain) > rect_right(rbound)) {
	rconstrain->r_left = rbound->r_left + rbound->r_width
	    - rconstrain->r_width;
    }
    if (rconstrain->r_left < rbound->r_left) {
	rconstrain->r_left = rbound->r_left;
    }
    if (rect_bottom(rconstrain) > rect_bottom(rbound)) {
	rconstrain->r_top = rbound->r_top + rbound->r_height
	    - rconstrain->r_height;
    }
    if (rconstrain->r_top < rbound->r_top) {
	rconstrain->r_top = rbound->r_top;
    }
}


static caddr_t
menu_return_result(m, mi, by_value)
	register struct menu *m;
	register struct menu_item *mi;
{   
    caddr_t v;
    
    if (range(m->selected_item, 1, m->nitems)) {
	mi = m->item_list[m->selected_item - 1];
	if (mi->pullright && !mi->notify_proc) {	/* C2:Just menu */
	    v = (((struct menu *)mi->value)->notify_proc
		 ? ((struct menu *)mi->value)->notify_proc
		 : by_value ? menu_return_value : menu_return_item)
		(mi->value, mi);
	    m->use_result = ((struct menu *)mi->value)->use_result;
	} else if (mi->notify_proc) {			/* C3:Notify proc */
	    m->use_result = FALSE;	/* Notify proc may change this */
	    v = (mi->notify_proc)(m, mi);
	} else {					/* C4:Result */
	    m->use_result = TRUE;
	    v = by_value ? (caddr_t)mi->value : (caddr_t)mi;
	}
    } else {  /* Follow default selection chain */	/* C5:Use default */
	m->selected_item = get_curitem(m, m->default_selection);
	/* m->use_result will be set after this call */
	v = (by_value ? menu_return_value : menu_return_item)(m, mi);
    }
    return v;
}


caddr_t
menu_return_value(m, mi)
	struct menu *m;
	struct menu_item *mi;
{   
    if (!m || !mi) {					/* C1:No menu or item */
	if (m) m->use_result = FALSE;
	return (caddr_t)MENU_NO_VALUE;
    }
    return menu_return_result(m, mi, TRUE);
}

    
caddr_t
menu_return_item(m, mi)
	struct menu *m;
	struct menu_item *mi;
{   
    if (!m || !mi) {					/* C1:No menu or item */
	if (m) m->use_result = FALSE;
	return MENU_NO_ITEM;
    }
    return menu_return_result(m, mi, FALSE);
}

    
caddr_t
menu_return_no_value(m, mi)
	struct menu *m;
	struct menu_item *mi;
{   
    m->use_result = FALSE;
    return (caddr_t)MENU_NO_VALUE;
}


caddr_t
menu_return_no_item(m, mi)
	struct menu *m;
	struct menu_item *mi;
{   
    m->use_result = FALSE;
    return (caddr_t)MENU_NO_ITEM;
}

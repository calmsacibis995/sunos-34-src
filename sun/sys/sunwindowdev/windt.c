#ifndef lint
static  char sccsid[] = "@(#)windt.c 1.8 87/03/16";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * SunWindows Desktop driver with multiple screens support.
 */

#include "../sunwindowdev/wintree.h"
#include "../sunwindow/cursor_impl.h"	/* for cursor access macros */
#include "../pixrect/pr_util.h"		/* for mpr support */
#include "../pixrect/pr_dblbuf.h" /* for double buffering support */
#include "../pixrect/pr_planegroups.h"	/* for plane groups */
#include "../h/file.h"			/* for FREAD and FWRITE */
#include "../sun/fbio.h"
#include "../sunwindow/pw_dblbuf.h"


extern	struct window *wt_intersected();
extern	int wt_notifychanged();
extern	bool wt_isindisplaytree();

void	dtop_putcolormap();

static void	dtop_init_data_lock();
static void	dtop_init_display_lock();
static void	dtop_init_mutex_lock();
static void	dtop_update_cursor();

static int	dtop_check_lock();

int wincursordisable;	/* Temp: Turns off displaying of the cursor */

int win_waiting_debug;	/* to printf when wakeup waiter */

void	dtop_change_loc_dtop();
void	dtop_update_enable();
void	dtop_set_enable();
void	dtop_flip_display();	/* Double buffering flip */
void	dtop_choose_dblbuf();
void	dtop_changedisplay();
int	dtop_isdblwin();
void	dtop_copy_dblbuffer();
int	dtop_cursordraw();
/*
 * The following should only be call when the display lock is off
 * or when you are not sure.
 */
dtop_cursorup(dtop)
	register Desktop *dtop;
{
	if (dtop && !(cursor_up(dtop) || 
	    horiz_hair_up(dtop) || vert_hair_up(dtop)) &&
	    (!win_lock_display_locked(dtop->shared_info)))
		dtop_drawcursor(dtop);
}

dtop_cursordown(dtop)
	register Desktop *dtop;
{
	if ((cursor_up(dtop) || horiz_hair_up(dtop) || vert_hair_up(dtop)) &&
	    (!win_lock_display_locked(dtop->shared_info)))
		dtop_drawcursor(dtop);
}

caddr_t
win_kmem_alloc(size)
	int size;
{
	caddr_t kmem_alloc();
	caddr_t ptr;
	
	ptr = kmem_alloc((u_int)size);
	bzero(ptr, (u_int)size);
	return (ptr);
}

dtop_openfb(dtop, fd)
	register Desktop *dtop;
	int	fd;
{
	register int	err = 0;
	struct	fbpixrect fbpr;
	struct	singlecolor *bkgnd = &dtop->dt_screen.scr_background;
	struct	singlecolor *frgnd = &dtop->dt_screen.scr_foreground;
	struct	singlecolor *tmpgnd;
	register struct	dtopcursor *dc;
	caddr_t zmemall(), dtop_alloccmapdata();
	int	memall();
	extern	caddr_t win_kmem_alloc();
	struct	fbgattr fbgattr;
	int	(*ioctl_op)();
	register int	i;
	int plane_group_count;
	int	max_depth = 1;

	if (fd == WSID_DEVNONE)
		return(0);
	/*
	 * Open framebuffer.
	 */
	err = kern_openfd(fd, &dtop->dt_fbfp, FWRITE|FREAD);
	if (err)
		return(err);
	ioctl_op = dtop->dt_fbfp->f_ops->fo_ioctl;
	/* Determine colormap size */
	err = (*ioctl_op)(dtop->dt_fbfp, FBIOGATTR, &fbgattr);
	if (err) {
		if (err != ENOTTY) {
			printf("ioctl err FBIOGATTR\n");
			return(err);
		}
		/* Use older ioctl if FBIOGATTR not implemented */
		err = (*ioctl_op)(dtop->dt_fbfp, FBIOGTYPE, &fbgattr.fbtype);
		if (err) {
			printf("ioctl err FBIOGTYPE\n");
			return(err);
		}
	}
	/*
	 * Colormap may not be less then 2 long (2 long is monochrome case).
	 * Will allow user programs to allocate colormap segments bigger
	 * than dt_cmsize.  However, colormap entries greater than the
	 * length of the map will be ignored.  This convention enables
	 * programs written for color displays to run without errors on
	 * a monochrome display.
	 */
	if ((dtop->dt_cmsize = fbgattr.fbtype.fb_cmsize) < 2) {
		printf("illegal colormap size (%D)\n",fbgattr.fbtype.fb_cmsize);
		return(EINVAL);
	}

	/*
	 * Get pixrect for framebuffer.
	 */
	err = (*ioctl_op)(dtop->dt_fbfp, FBIOGPIXRECT, &fbpr);
	if (err) {
		printf("ioctl err FBIOGPIXRECT\n");
		return(err);
	}
	dtop->dt_pixrect = fbpr.fbpr_pixrect;
	if (dtop->dt_pixrect==(struct pixrect *)0)
		return (EBUSY);
	if (dtop->dt_pixrect==(struct pixrect *)-1)
		return (EINVAL);


	/* Determine double buffering capabilities */
	if ( (err = pr_dbl_get(dtop->dt_pixrect, PR_DBL_AVAIL))
			 == PR_DBL_EXISTS ) {
		dtop->dt_flags |= DTF_DBLBUFFER;
		dtop->dt_curdbl = WINDOW_NULL;
		dtop->dt_dbl_bkgnd = PR_DBL_B;
		dtop->dt_dbl_frgnd = PR_DBL_A;
		dtop->dt_curdbl = 0;
	}

	/* Allocate colormap resource map */
	dtop->dt_cmapdata = dtop_alloccmapdata(dtop, &dtop->dt_cmap,
	    dtop->dt_cmsize);
	if (dtop->dt_cmapdata == 0)
		return(ENXIO);
	if ((dtop->dt_rmp = (struct mapent *) win_kmem_alloc(
	    sizeof(struct mapent)*CRM_SIZE(dtop->dt_cmsize))) == 0) {
		printf("Couldn't allocate colormap resource map\n");
		return(ENXIO);
	}
	rminit((struct map *)dtop->dt_rmp,
	    (long)(CRM_NSLOTS(dtop->dt_cmsize)*CRM_SLOTSIZE),
	    (long)(0+CRM_ADDROFFSET), "colormap", CRM_SIZE(dtop->dt_cmsize));

	/* Determine foreground and background of color map */
	if (dtop->dt_screen.scr_flags & SCR_SWITCHBKGRDFRGRD) {
		tmpgnd = bkgnd;
		bkgnd = frgnd;
		frgnd = tmpgnd;
	}

	/* Determine available plane groups */
	(void) pr_available_plane_groups(dtop->dt_pixrect,DTOP_MAX_PLANE_GROUPS,
	    dtop->dt_plane_groups_available);
	/* Determine if multiple plane groups */
	plane_group_count = 0;
	for (i = 0; i < DTOP_MAX_PLANE_GROUPS; i++) {
		/* Restrict frame buffer plane group access */
		if (((dtop->dt_screen.scr_flags & SCR_8BITCOLORONLY) &&
		    (i != PIXPG_8BIT_COLOR)) ||
		    ((dtop->dt_screen.scr_flags & SCR_OVERLAYONLY) &&
		    (i != PIXPG_OVERLAY)))
			dtop->dt_plane_groups_available[i] = 0;
		if (dtop->dt_plane_groups_available[i]) {
			int end;

			plane_group_count++;
			/* Don't set e & background for enable plane */
			if (i != PIXPG_OVERLAY_ENABLE) {
				/* Set fore & background in each plane group */
				pr_set_plane_group(dtop->dt_pixrect, i);
				pr_putcolormap(dtop->dt_pixrect, 0, 1,
				    &bkgnd->red, &bkgnd->green, &bkgnd->blue);
				switch (dtop->dt_pixrect->pr_depth) {
				case 1: end = 1; break;
				case 8: end = 255; max_depth = 8; break;
				default: printf("Unknown depth %D\n",
				    dtop->dt_pixrect->pr_depth);
				}
				pr_putcolormap(dtop->dt_pixrect, end, 1,
				    &frgnd->red, &frgnd->green, &frgnd->blue);
			}
		}
	}
	if (plane_group_count > 1)
		dtop->dt_flags |= DTF_MULTI_PLANE_GROUPS;
	/* Set initial plane group explicitly */
	if ((dtop->dt_screen.scr_flags & SCR_8BITCOLORONLY) &&
	    dtop->dt_plane_groups_available[PIXPG_8BIT_COLOR])
		pr_set_plane_group(dtop->dt_pixrect, PIXPG_8BIT_COLOR);
	else if ((dtop->dt_screen.scr_flags & SCR_OVERLAYONLY) &&
	    dtop->dt_plane_groups_available[PIXPG_OVERLAY])
		pr_set_plane_group(dtop->dt_pixrect, PIXPG_OVERLAY);
	else if (dtop->dt_plane_groups_available[PIXPG_OVERLAY])
		/*
		 * PIXPG_OVERLAY takes precedence over PIXPG_8BIT_COLOR
		 * for backwards compatibility reasons (see
		 * win_pick_plane_group).
		 */
		pr_set_plane_group(dtop->dt_pixrect, PIXPG_OVERLAY);
	else if (dtop->dt_plane_groups_available[PIXPG_8BIT_COLOR])
		pr_set_plane_group(dtop->dt_pixrect, PIXPG_8BIT_COLOR);

	/*
	 * Set up the cross hair pixrects
	 */

	dc = &dtop->dt_cursor;

	dc->horiz_hair_mpr.pr_depth = max_depth;
	dc->horiz_hair_mpr.pr_width = dtop->dt_screen.scr_rect.r_width;
	dc->horiz_hair_mpr.pr_height = CURSOR_MAX_HAIR_THICKNESS;
	dc->horiz_hair_data.md_linebytes =
	    mpr_linebytes(dc->horiz_hair_mpr.pr_width,
			  dc->horiz_hair_mpr.pr_depth);

	dc->horiz_hair_size = 
	    dc->horiz_hair_mpr.pr_height * dc->horiz_hair_data.md_linebytes;
	dc->horiz_hair_data.md_image = (short *)win_kmem_alloc(
	    dc->horiz_hair_size);
	horiz_hair_set_up(dtop, FALSE);
	
	dc->vert_hair_mpr.pr_depth = max_depth;
	dc->vert_hair_mpr.pr_width = CURSOR_MAX_HAIR_THICKNESS;
	dc->vert_hair_mpr.pr_height = dtop->dt_screen.scr_rect.r_height;
	dc->vert_hair_data.md_linebytes = 
	   mpr_linebytes(dc->vert_hair_mpr.pr_width, 
			 dc->vert_hair_mpr.pr_depth);

	dc->vert_hair_size = 
	    dc->vert_hair_mpr.pr_height * dc->vert_hair_data.md_linebytes;
	dc->vert_hair_data.md_image = (short *)win_kmem_alloc(
	    dc->vert_hair_size);
	vert_hair_set_up(dtop, FALSE);

	if (!dc->horiz_hair_data.md_image || !dc->vert_hair_data.md_image) {
		printf("Couldn't allocate cross hair data storage\n");
		return(ENXIO);
	}

	/* Set up shared memory lock block */
	dtop->shared_info = (Win_lock_block *) 
	    zmemall(memall, sizeof(Win_lock_block));

	if (!dtop->shared_info) {
		printf("Couldn't allocate shared lock block storage\n");
		return(ENXIO);
	}

	dtop->shared_info->version = WIN_LOCK_VERSION;
	/* NOTE: maybe RISC instead */
	dtop->shared_info->cpu_type = WIN_LOCK_68000;
	return (0);
}

dtop_close(dtop)
	register Desktop *dtop;
{
	register Desktop *dt;
	register int p;
	Desktop *dtopneighbor = NULL;
	Desktop **dt_prev_ptr;
	register Workstation *ws = dtop->dt_ws;

	/*
	 * Remove references to this desktop from other desktops
	 */
	for (dt = desktops; dt < &desktops[NDTOP]; dt++)
		if (dt->dt_flags&DTF_PRESENT) {
			if (dt != dtop && dt->dt_ws == dtop->dt_ws)
				dtopneighbor = dt;
			for (p = 0; p < SCR_POSITIONS; p++)
				if (dt->dt_neighbors[p] == dtop)
					dt->dt_neighbors[p] = NULL;
		}
	/* Handle null workstation */
	if (ws == WORKSTATION_NULL)
		goto NoWs;
	/*
	 * Remove locator from this desktop
	 */
	dtop_change_loc_dtop(dtop, dtopneighbor, 0, 0);
	/* Remove desktop from workstation */
	dt_prev_ptr = &ws->ws_dtop;
	for (dt = ws->ws_dtop; dt; dt = dt->dt_next) {
		if (dt == dtop) {
			*dt_prev_ptr = dt->dt_next;
			break;
		}
		dt_prev_ptr = &dt->dt_next;
	}
	/* Close workstation if no more desktops using it */
	if (ws->ws_dtop == DESKTOP_NULL)
		ws_close(ws);

NoWs:
	/* Reset double buffering states */
	if ( dtop->dt_flags & DTF_DBLBUFFER ) {
		dtop_changedisplay(dtop, PR_DBL_A, PR_DBL_B, FALSE);
		if (pr_dbl_set(dtop->dt_pixrect, PR_DBL_WRITE, PR_DBL_BOTH, 0)) 
			printf("Error pr_dbl_set display  in dtop_close\n");
	}

	/*
	 * Close frame buffer.
	 */
	if (dtop->dt_fbfp) {
		/*
		 * DON'T clear screen because at this point have no way to
		 * arbitrate kernel access to the screen.  In the case of the
		 * sun1bw frame buffer could clobber some important register.
		 */
		/*
		 * Closing frame buffer will release dtop->dt_pixrect.
		 */
		closef(dtop->dt_fbfp);
	}
	/*
	 * Remove allocated resources
	 */
	rl_free(&dtop->dt_rlinvalid);
	cms_freecmapdata(dtop->dt_cmapdata, &dtop->dt_cmap,
	    dtop->dt_cmsize);
	if (dtop->dt_rmp)
		kmem_free((caddr_t)dtop->dt_rmp,
		    (u_int)CRM_SIZE(dtop->dt_cmsize));
	/* Release lock static data */
	wlok_done(&dtop->dt_displaylock);
	wlok_done(&dtop->dt_datalock);
	wlok_done(&dtop->dt_mutexlock);

	/* free the cross hair storage */
	if (dtop->dt_cursor.horiz_hair_data.md_image)
		kmem_free((caddr_t) dtop->dt_cursor.horiz_hair_data.md_image, 
		    (u_int)dtop->dt_cursor.horiz_hair_size);
	if (dtop->dt_cursor.vert_hair_data.md_image)
		kmem_free((caddr_t) dtop->dt_cursor.vert_hair_data.md_image, 
		    (u_int)dtop->dt_cursor.vert_hair_size);
	rl_free(&dtop->dt_cursor.horiz_hair_rectlist);
	rl_free(&dtop->dt_cursor.vert_hair_rectlist);

	/* free the shared lock block */
	wmemfree((caddr_t) dtop->shared_info, sizeof(Win_lock_block));

	bzero((caddr_t)dtop, sizeof (*dtop));
}

void
dtop_change_loc_dtop(dtop, dtop_new, x, y)
	register Desktop *dtop;
	register Desktop *dtop_new;
	int x, y;
{
	register Workstation *ws = dtop->dt_ws;

	if (dtop == ws->ws_loc_dtop) {
		dtop_cursordown(dtop);
		win_shared_update_cursor_active(dtop->shared_info, FALSE);
		ws->ws_loc_dtop = dtop_new;
		if (dtop_new) {
			dtop_new->dt_rt_x = x;
			dtop_new->dt_rt_y = y;
			dtop_new->shared_info->status.new_cursor = TRUE;
			/* update the shared memory */
			win_shared_update_mouse_xy(dtop_new->shared_info, 0, 0);
			win_shared_update_cursor_active(
			    dtop_new->shared_info, TRUE);
			dtop_update_enable(dtop_new, dtop, 0);
		}
	}
	/* Remove pick focus from this desktop */
	if (dtop == ws->ws_pick_dtop) {
		ws->ws_pick_dtop = dtop_new;
		if (dtop_new) {
			dtop_new->dt_ut_x = x;
			dtop_new->dt_ut_y = y;
		}
	}
}

void
dtop_update_enable(dtop_new, dtop, doit)
	register Desktop *dtop_new;
	Desktop *dtop;
	int doit;
{
	char groups[DTOP_MAX_PLANE_GROUPS];
	register struct pixrect *pr;

	if (dtop_new == DESKTOP_NULL)
		return;
	/* Find a pixrect that is capable of setting the enable plane */
	pr = dtop_new->dt_pixrect;
	(void) pr_available_plane_groups(pr, DTOP_MAX_PLANE_GROUPS, groups);
	if (!groups[PIXPG_OVERLAY_ENABLE]) {
		if (dtop == DESKTOP_NULL)
			return;
		pr = dtop->dt_pixrect;
		(void) pr_available_plane_groups(pr, DTOP_MAX_PLANE_GROUPS,
		    groups);
		if (!groups[PIXPG_OVERLAY_ENABLE])
			return;
	}
	/* See if SCR_TOGGLEENABLE set anywhere */
	if ((dtop_new->dt_screen.scr_flags & SCR_TOGGLEENABLE) ||
	    (dtop && (dtop->dt_screen.scr_flags & SCR_TOGGLEENABLE)) || doit)
		dtop_set_enable(dtop_new, pr);
}

void
dtop_set_enable(dtop, pr)
	Desktop *dtop;
	register struct pixrect *pr;
{
	int original_plane_group;

	original_plane_group = pr_get_plane_group(pr);
	pr_set_planes(pr, PIXPG_OVERLAY_ENABLE, PIX_ALL_PLANES);
	if (dtop->dt_plane_groups_available[PIXPG_OVERLAY] ||
	    dtop->dt_plane_groups_available[PIXPG_MONO])
		pr_rop(pr, 0, 0, pr->pr_width, pr->pr_height,
		    PIX_SET, 0, 0, 0);
	if (dtop->dt_plane_groups_available[PIXPG_8BIT_COLOR])
		pr_rop(pr, 0, 0, pr->pr_width, pr->pr_height,
		    PIX_CLR, 0, 0, 0);
	pr_set_planes(pr, original_plane_group, PIX_ALL_PLANES);
}

int
dtop_removewin(dtop, w)
	Desktop *dtop;
	register struct window *w;
{
	int	error;
	bool	fixclipping;

	if (w->w_link[WL_PARENT] == NULL)
		return (EINVAL);
	fixclipping = wt_isindisplaytree(w);
	if ((error = wt_remove(w)) < 0)
		return (error);
	if (fixclipping)
		dtop_invalidateclipping(dtop, w->w_link[WL_PARENT], &w->w_rect);
	return (0);
}

dtop_interrupt(dtop, poll_rate)
	register Desktop *dtop;
	int poll_rate;
{
	register Win_lock_block	*shared_info = dtop->shared_info;

	/* Do lock timeout resolution */
	if (win_lock_data_locked(shared_info))
		wlok_lockcheck(&dtop->dt_datalock, poll_rate, 0);

	/* checked the shared memory locks */

	dtop_check_lock(dtop, &dtop->dt_mutexlock, &shared_info->mutex, 
	    poll_rate, dtop_init_mutex_lock);

	dtop_check_lock(dtop, &dtop->dt_displaylock, &shared_info->display, 
	    poll_rate, dtop_init_display_lock);

	/* See if moved or new cursor that should be put up now */
	if (shared_info->status.new_cursor) {
		dtop_cursordown(dtop);
		/*
		dtop->dt_flags &= ~DTF_NEWCURSOR;
		*/
		dtop_cursorup(dtop);
	}

	/* see if a process is waiting for an already released lock.
	 * We need to do this here because of the race condition that 
	 * exists between when a user process checks the waiting count
	 * and when the kernel sets it before sleeping.
	 */
	if ((dtop->display_waiting) && !win_lock_display_locked(shared_info)) {
		if (win_waiting_debug)
			printf("Waking up waiting process because display lock is free\n");
		wakeup((caddr_t)shared_info);
	}

	/* see if shared locking info needs updating */
	win_shared_update(dtop);
}


dtop_validate_shared_lock(dtop, wlock)
	register Desktop 	*dtop;
	Winlock			*wlock;
{
	register Win_lock_block	*shared_info = dtop->shared_info;

	if (wlock == &dtop->dt_displaylock)
	    dtop_check_lock(dtop, wlock, &shared_info->display,
	    0, dtop_init_display_lock);
	else
	    dtop_check_lock(dtop, wlock, &shared_info->mutex,
	    0, dtop_init_mutex_lock);

	/* see if shared locking info needs updating */
	win_shared_update(dtop);
}


static int
dtop_check_lock(dtop, wlock, shared_lock, poll_rate, init_lock)
	Desktop		*dtop;
	Winlock		*wlock;
	Win_lock	*shared_lock;
	int		poll_rate;
	void		(*init_lock)();

{
	int	orig_count;

	if (!win_lock_locked(shared_lock)) {
	    if (wlock->lok_pid) {
		/* Here the user has unlocked the mutex lock.
		 * So we clear the kernel lock.
		 * First set the lock bit so we can unlock it with
		 * wlok_unlock().
		 */
		win_lock_set(shared_lock, TRUE);
		shared_lock->count = 1;
		wlok_unlock(wlock);
	    }
	    return;
	}

	/* the shared lock is locked */

	if (wlock->lok_id && (wlock->lok_id != shared_lock->id)) {
	    /* Here a user process has reacquired the lock.
	     * So we forget about the old lock.
	     * Don't do the usual unlock action, since the lock
	     * is still locked.
	     * Also, be sure to preserve the lock count.
	     */
	    orig_count = shared_lock->count;
	    shared_lock->count = 1;
	    wlock->lok_unlock_action = 0;
	    wlok_unlock(wlock);
	    /* reset the lock, since wlok_unlock() zero'ed it */
	    win_lock_set(shared_lock, TRUE);
	    shared_lock->count = orig_count;
	}

	if (wlock->lok_pid) {
	    /* Here the lock is set and the timer is
	     * running, so we check the lock.
	     */
	    if (poll_rate)
		wlok_lockcheck(wlock, poll_rate, 0);
	    return;
	}

	/* Here the lock was set by a user process,
	 * so we need to initialize our internal lock.
	 */
	if (shared_lock->pid) {
	    extern struct proc 	*pfind();
	    struct proc		*process = pfind(shared_lock->pid);

	    orig_count = shared_lock->count;
	    wlok_clear(wlock);
	    (init_lock)(dtop, shared_lock->id, process);
	    shared_lock->count = orig_count;
	    if (!process) {
		/* Here a user process has set the lock,
		 * and either the process has died, or the pid was
		 * not set correctly.  So we clear the lock.
		 */
		if (!(wlock->lok_options & WLOK_SILENT))
		    printf("Window %s lock broken after process %D went away\n",
			   wlock->lok_string, shared_lock->pid);
		wlok_forceunlock(wlock);
	    }
	} else {
	    /* Here the lock bit is set, but the process has
	     * not set the lock.pid yet.
	     * So complain after a while.
	     */
	    if (wlock->lok_time == 0) {
		 /* initialize the lock */
		orig_count = shared_lock->count;
		wlok_clear(wlock);
		(init_lock)(dtop, shared_lock->id, 0);
		shared_lock->count = orig_count;
	    }
	    if (poll_rate)
		wlok_lockcheck(wlock, poll_rate, 0);
	}
}


/*
 * Constrains to be on desktop and moves to another if adjacent.
 */
dtop_track_locator(dtop_ptr, x_ptr, y_ptr)
	Desktop	**dtop_ptr;
	int	*x_ptr;
	int	*y_ptr;
{
	register Desktop *dtop = *dtop_ptr;
	register Desktop *dtop_old = *dtop_ptr;
	register int x = *x_ptr;
	register int y = *y_ptr;

	if (x < dtop->dt_screen.scr_rect.r_left)
		if (dtop->dt_neighbors[SCR_WEST]) {
			dtop = dtop->dt_neighbors[SCR_WEST];
			x = dtop->dt_screen.scr_rect.r_left+
			    dtop->dt_screen.scr_rect.r_width-
			    (dtop_old->dt_screen.scr_rect.r_left-x);
			y = (dtop->dt_screen.scr_rect.r_height*
			    y)/dtop_old->dt_screen.scr_rect.r_height;
		} else
			x = dtop->dt_screen.scr_rect.r_left;
	else if (x > dtop->dt_screen.scr_rect.r_width)
		if (dtop->dt_neighbors[SCR_EAST]) {
			dtop = dtop->dt_neighbors[SCR_EAST];
			x = dtop->dt_screen.scr_rect.r_left+
			    (x-dtop_old->dt_screen.scr_rect.r_width);
			y = (dtop->dt_screen.scr_rect.r_height*
			    y)/dtop_old->dt_screen.scr_rect.r_height;
		} else
			x = rect_right(&dtop->dt_screen.scr_rect);
	if (y < dtop->dt_screen.scr_rect.r_top)
		if (dtop->dt_neighbors[SCR_NORTH]) {
			dtop = dtop->dt_neighbors[SCR_NORTH];
			y = dtop->dt_screen.scr_rect.r_top+
			    dtop->dt_screen.scr_rect.r_height-
			    (dtop_old->dt_screen.scr_rect.r_top-y);
			x = (dtop->dt_screen.scr_rect.r_width*
			    x)/dtop_old->dt_screen.scr_rect.r_width;
		} else
			y = dtop->dt_screen.scr_rect.r_top;
	else if (y > dtop->dt_screen.scr_rect.r_height)
		if (dtop->dt_neighbors[SCR_SOUTH]) {
			dtop = dtop->dt_neighbors[SCR_SOUTH];
			y = dtop->dt_screen.scr_rect.r_top+
			    (y-dtop_old->dt_screen.scr_rect.r_height);
			x = (dtop->dt_screen.scr_rect.r_width*
			    x)/dtop_old->dt_screen.scr_rect.r_width;
		} else
			y = rect_bottom(&dtop->dt_screen.scr_rect);
	*dtop_ptr = dtop;
	*x_ptr = x;
	*y_ptr = y;
}

dtop_set_cursor(dtop)
	register Desktop *dtop;
{
	Workstation *ws = dtop->dt_ws;
	register struct	window *oldcursorwin = dtop->dt_cursorwin;
	register struct	window *newcursorwin;

	dtop->dt_cursorwin = NULL;
	if ((ws == WORKSTATION_NULL) || (dtop->dt_rootwin == NULL) ||
	    (ws->ws_loc_dtop != dtop))
		return;
	if (ws->ws_inputgrabber) {
		/* The cursor reflects the input grabber */
		dtop->dt_cursorwin = ws->ws_inputgrabber;
	} else {
		/* The cursor reflects underlaying window */
		dtop->dt_cursorwin = wt_intersected(dtop->dt_rootwin,
		    dtop->dt_rt_x, dtop->dt_rt_y);
	}
	newcursorwin = dtop->dt_cursorwin;
	/* Might need new cursor up because of change of underlaying window */
	/*
	dtop->dt_flags |= DTF_NEWCURSOR;
	*/
	dtop->shared_info->status.new_cursor = TRUE;

	/* Change colormap if need to */
	if ((dtop->dt_flags & DTF_NEWCMAP ||
	    (oldcursorwin != newcursorwin))) {
		if (newcursorwin && (newcursorwin->w_flags & WF_CMSHOG)) {
			dtop_putcolormap(dtop, newcursorwin->w_cms.cms_size,
			    &newcursorwin->w_cmap);
		} else if (dtop->dt_flags & DTF_NEWCMAP ||
		    (oldcursorwin && (oldcursorwin->w_flags & WF_CMSHOG))) {
			dtop_putcolormap(dtop, dtop->dt_cmsize, &dtop->dt_cmap);
		}
		dtop->dt_flags &= ~DTF_NEWCMAP;
	}
	/* update the shared cursor if the cursor window or mouse window
	 * has changed.
	 */
	if (oldcursorwin != newcursorwin)
		win_shared_update_cursor(dtop);
}

int
dtop_lockdata(dev)
	dev_t	dev;
{
	Window *w = winfromdev(dev);
	register Desktop *dtop = w->w_desktop;
	register Win_lock_block	*shared_info = dtop->shared_info;

	if (!win_lock_data_locked(dtop->shared_info)) {
		/* mark the shared memory to show that
		 * someone is wating for a lock.
		 */
		shared_info->waiting++;
		dtop->display_waiting++;

		while (
		    win_lock_mutex_locked(shared_info) ||
		    win_lock_data_locked(shared_info) ||
		    (win_lock_display_locked(shared_info) &&
		    (shared_info->display.pid!=u.u_procp->p_pid)))
			if(sleep((caddr_t)dtop->shared_info, LOCKPRI|PCATCH)) {
				shared_info->waiting--;
				dtop->display_waiting--;
				return (-1);
			}

		/* unmark the shared memory lock */
		shared_info->waiting--;
		dtop->display_waiting--;

		dtop_init_data_lock(dtop, u.u_procp);
		shared_info->data.pid = dtop->dt_datalock.lok_pid;
	} else {
		/*
		 * Wouldn't get into this routine unless
		 * calling pid == dt_datalock.lok_pid
		 */
		*(dtop->dt_datalock.lok_count) += 1;
	}
	return (0);
}

void
dtop_timedout_data(wlock)
	Winlock *wlock;
{
	extern	struct proc *pfind();
	register struct proc *process = pfind(wlock->lok_pid);

	/* Send SIGXCPU if process still exists */
	if (process != (struct proc *)0) {
		psignal(process, SIGXCPU);
		printf("The offending process was sent SIGXCPU\n");
	}
}

void
dtop_unlockdata(wlock)
	Winlock *wlock;
{
	Desktop	*dtop = (Desktop *)wlock->lok_client;

	dtop_computeclipping(dtop, TRUE);
	dtop->shared_info->data.pid = 0;
}

dtop_computeclipping(dtop, notify)
	register Desktop *dtop;
	bool	notify;
{
	if (dtop->dt_parentinvalidwin == NULL)
		return;
	wt_setclipping(dtop->dt_parentinvalidwin, &dtop->dt_rlinvalid);
	ws_set_focus(dtop->dt_ws, FF_PICK_CHANGE);
	dtop_set_cursor(dtop);
	rl_free(&dtop->dt_rlinvalid);
	if (notify) {
		(void) wt_enumeratechildren(wt_notifychanged, 
		    dtop->dt_parentinvalidwin, (struct rect *)0);
		dtop->dt_parentinvalidwin = NULL;
	}
}

dtop_invalidateclipping(dtop, parent, rect)
	register Desktop *dtop;
	register struct window *parent;
	struct	rect *rect;
	/*
	 * The parent relative rect has invalid clipping and
	 * parent is the bottom most window affected.
	 */
{
	struct	rectlist rlparent;
	struct	window *root = parent->w_desktop->dt_rootwin;

	rl_initwithrect(rect, &rlparent);
	/*
	 * Make invalid stuff relative to root.
	 * Need to do this because eventual call to wt_setclipping must be
	 * called with root as the initial window.
	 */
	while (parent != root) {
		wt_passrltoancestor(&rlparent, parent,
		    parent->w_link[WL_PARENT]);
		parent = parent->w_link[WL_PARENT];
	}
	dtop->dt_parentinvalidwin = root;
	rl_union(&rlparent, &dtop->dt_rlinvalid, &dtop->dt_rlinvalid);
	rl_free(&rlparent);
	if (!win_lock_data_locked(dtop->shared_info))
		dtop_computeclipping(dtop, TRUE);
}


/*
 * Display access control
 */
int
dtop_lockdisplay(dev, rect)
    dev_t	dev;
    struct	rect *rect;
{
    register struct window 	*w = winfromdev(dev);
    register Desktop		*dtop = w->w_desktop;
    register Win_lock_block	*shared_info = dtop->shared_info;
    struct rect 		rectcursor, screen_rect;
    short			take_down_cursor = FALSE;

    /*
    * Currently, locking a dev you have locked increments a ref count.
    */
    if (!win_lock_display_locked(shared_info)) {
	/* mark the shared memory to show that
	 * someone is wating for a lock.
	 */
	shared_info->waiting++;
	dtop->display_waiting++;

	/* loop until the lock is free */
	while ( win_lock_mutex_locked(shared_info) ||
		win_lock_display_locked(shared_info) ||
		((dtop->dt_displaygrabber != NULL) &&
		(dtop->dt_displaygrabber->w_pid != w->w_pid)) ||
		(win_lock_data_locked(shared_info) &&
		 (dtop->dt_datalock.lok_pid != u.u_procp->p_pid))
	      )
		if (sleep((caddr_t)dtop->shared_info, LOCKPRI|PCATCH)) {
	    		shared_info->waiting--;
			dtop->display_waiting--;
			return (-1);
		}

	/* unmark the shared memory lock */
	shared_info->waiting--;
	dtop->display_waiting--;

	if (cursor_up(dtop)) {
	    int cursor_in;	/* Whether the cursor is in w ? */

	    rect_construct(&rectcursor,
			    cursor_x(dtop) - w->w_screenx,
			    cursor_y(dtop) - w->w_screeny,
			    cursor_screen_width(dtop),
			    cursor_screen_height(dtop));

	    cursor_in = rl_rectintersects(&rectcursor, &w->w_rlexposed);
	    /*
	     * Don't take down the cursor if the cursor is in the 
             * currently active double buffering window	
	     */
	    if ((dtop->dt_curdbl == w) && cursor_in)
		take_down_cursor = FALSE;
	    else take_down_cursor = 
			rect_intersectsrect(rect, &rectcursor) &&
			(dtop->dt_displaygrabber == w || cursor_in);
	    /*
	     * Can't restrict cursor removal if
	     * displaygrabbed because user may violate
	     * w_rlexposed if wants, i.e., to do menus.
	     */
	}

	/* convert locked area to screen coordinates */
	screen_rect = *rect;
	rect_passtoparent(w->w_screenx, w->w_screeny, &screen_rect);

	/* check for intersection with cross hairs */
	if (horiz_hair_up(dtop)) {
	    take_down_cursor = take_down_cursor ||
		rl_rectintersects(&screen_rect, 
				  &dtop->dt_cursor.horiz_hair_rectlist);
	}

	if (vert_hair_up(dtop)) {
	    take_down_cursor = take_down_cursor ||
		rl_rectintersects(&screen_rect, 
				  &dtop->dt_cursor.vert_hair_rectlist);
	}

	if (take_down_cursor)
	    dtop_cursordown(dtop);

	shared_info->display.pid = u.u_procp->p_pid;
	shared_info->display.id++;
	dtop_init_display_lock(dtop, shared_info->display.id, u.u_procp);

	/* Double buffer support 
	* Flip the write control bits  if current window is the
	*  double bufferer else set write to both
	*/
	if (dtop->dt_dblcount) {
		if (dtop->dt_curdbl == w ) {
			if (pr_dbl_set(dtop->dt_pixrect, PR_DBL_WRITE, 
				dtop->dt_dbl_bkgnd,PR_DBL_READ, 
					dtop->dt_dbl_bkgnd,0))
				printf("Error pr_dbl_set lock display\n");
		}
		else if (!(dtop->dt_displaygrabber == w)) {
			if(pr_dbl_set(dtop->dt_pixrect,PR_DBL_WRITE,PR_DBL_BOTH,
			   PR_DBL_READ, dtop->dt_dbl_frgnd,0))
				printf("Error dbl_set lock display not", 
				" a cur dbl %d\n", dtop->dt_dbl_frgnd);
		}
	}
    } else if (dtop->dt_displaylock.lok_count)
	*(dtop->dt_displaylock.lok_count) += 1;
    else
	printf("Warning: display lock count pointer is NULL\n");
    /*
    * Note: Should check to see if going to write outside of
    * original rect affected.
    */
    return (w->w_clippingid);
}

/* ARGSUSED */
void
dtop_timedout_display(wlock)
	Winlock *wlock;
{
	Desktop		*dtop = (Desktop *) wlock->lok_client;
	Win_lock	*shared_lock = &dtop->shared_info->display;

	printf("You may see display garbage because of this action\n");

	/* debug info */
	if ((wlock->lok_pid != shared_lock->pid) || 
	    (wlock->lok_id != shared_lock->id)) {
		printf("Internal lock: pid = %d, id = %d, time = %d\n",
	            wlock->lok_pid, wlock->lok_id, wlock->lok_time);
		printf("Shared lock: pid = %d, id = %d\n",
	    	    shared_lock->pid, shared_lock->id);
	}
}

static void
dtop_update_cursor(dtop)
	register Desktop *dtop;
{

        if (cursor_up(dtop) || horiz_hair_up(dtop) || vert_hair_up(dtop)) {
		if (dtop != dtop->dt_ws->ws_loc_dtop)
			/*
			 * take down current cursor ms not on dtop anymore.
			 */
			dtop_cursordown(dtop);
		if (dtop->dt_cursorwin &&
		    ((cursor_x(dtop) != dtop->dt_rt_x -
			dtop->dt_cursorwin->w_cursor.cur_xhot) ||
		    (cursor_y(dtop) != dtop->dt_rt_y -
			dtop->dt_cursorwin->w_cursor.cur_yhot)))
			/*
			 * take down current cursor cause in wrong position
			 */
			dtop_cursordown(dtop);
	}
}

void
dtop_unlockdisplay(wlock)
	Winlock *wlock;
{
	register Desktop *dtop = (Desktop *)wlock->lok_client;

	dtop_update_cursor(dtop);
#if 0
if (dtop->dt_dblcount)
printf("unlock %d\n",dtop->shared_info->display.pid);
#endif
	dtop->shared_info->display.pid = 0;
	/*
	 * put up current cursor now
	 */
	dtop_cursorup(dtop);
}

void
dtop_unlockmutex(wlock)
	Winlock *wlock;
{
	register Desktop *dtop = (Desktop *)wlock->lok_client;

	dtop->shared_info->mutex.pid = 0;
}

dtop_drawcursor(dtop)
    register Desktop *dtop;
{
#define	ROPERR(tag) {rop_err_id = (tag); goto Roperr;}
    register struct dtopcursor 	*dc;
    register struct cursor 	*cursor;
    register struct mpr_data 	*scrmpr_data;
    struct mpr_data 		*data;
    struct pixrect		*shape;
    int				planes;	/* Can't be register */
    struct rect 		hair_rect;
    short			want_cursor;
    Workstation			*ws;
    Win_shared_cursor		*shared;
    struct window		*cursorwin;
    struct pixrect		*pr;
    register int		full_planes = PIX_ALL_PLANES;
    register int		original_plane_group;
    register int		rop_err_id = 0;
    int                         readstate;
    int                         writestate;

    if (dtop == NULL || dtop->dt_pixrect == NULL || wincursordisable)
	return;

    /* can't do anything if the shared structure is locked down */
    if (win_lock_mutex_locked(dtop->shared_info))
	return;

    /* Save the read/write control states */
    /* Set read/write control bits to read/write from the foreground */
    if (dtop->dt_dblcount) {
	readstate = pr_dbl_get(dtop->dt_pixrect, PR_DBL_READ);
        writestate = pr_dbl_get(dtop->dt_pixrect, PR_DBL_WRITE);
	if (pr_dbl_set(dtop->dt_pixrect, PR_DBL_WRITE, dtop->dt_dbl_frgnd,
			PR_DBL_READ, dtop->dt_dbl_frgnd, 0))
		printf("Error dbl_set draw cursor\n");
    }

    shared = &dtop->shared_info->cursor_info;
    dc = &dtop->dt_cursor;
    pr = dtop->dt_pixrect;
    original_plane_group = pr_get_plane_group(pr);

    if (cursor_up(dtop) || enable_plane_cursor_up(dtop) ||
	horiz_hair_up(dtop) || vert_hair_up(dtop)) {

	/* Set up plane group to be the one that holds the screen image */
	pr_set_planes(pr, shared->plane_group, full_planes);

	/*
	* Take existing cursor down by putting old screen image there
	*/
	if (cursor_up(dtop)) {
	    /* prepare the screen pixrect for use */
	    win_lock_prepare_screen_pr(shared);
	    cursor_set_up(dtop, FALSE);
	    /* rop up the saved bits */
	    if (pr_rop(pr, shared->x, shared->y,
		       shared->screen_pr.pr_width, shared->screen_pr.pr_height,
                       PIX_SRC, &shared->screen_pr, 0, 0))
		ROPERR(1)
	}
	/* replace bits in enable plane */
	if (enable_plane_cursor_up(dtop)) {
	    enable_plane_cursor_set_up(dtop, FALSE);
	    pr_set_planes(pr, PIXPG_OVERLAY_ENABLE, full_planes);
	    if (pr_rop(pr, shared->x, shared->y,
	       dc->enable_mpr.pr_width, dc->enable_mpr.pr_height,
	       PIX_SRC, &dc->enable_mpr, 0, 0))
		ROPERR(2)
	    pr_set_planes(pr, original_plane_group, full_planes);
	}

	/* replace image under cross hairs */
	if (horiz_hair_up(dtop)) {
	    horiz_hair_set_up(dtop, FALSE);
	    if (dtop_rl_rop(pr, 0, 0, 
	                    &dc->horiz_hair_rectlist,
	                    PIX_SRC, &dc->horiz_hair_mpr,
	                    0, dc->hair_y))
		ROPERR(3)
	    rl_free(&dc->horiz_hair_rectlist);
	}

	if (vert_hair_up(dtop)) {
	    vert_hair_set_up(dtop, FALSE);
	    if (dtop_rl_rop(pr, 0, 0,
	                    &dc->vert_hair_rectlist,
	                    PIX_SRC, &dc->vert_hair_mpr,
	                    dc->hair_x, 0))
		ROPERR(4)
	    rl_free(&dc->vert_hair_rectlist);
	}

	goto Done;
    }

    /*
    * Put up new cursor by saving old screen image then writing
    * new cursor.
    */
    cursorwin = dtop->dt_cursorwin;
    if (!cursorwin || !(ws = dtop->dt_ws) || (ws->ws_loc_dtop != dtop))
	goto Done;

    /* we are dealing with the latest cursor */
    dtop->shared_info->status.new_cursor = FALSE;

    cursor = &cursorwin->w_cursor;
    data = &cursorwin->w_cursordata;
    shape = cursor->cur_shape;

    want_cursor = shape->pr_width && shape->pr_height &&
			shape->pr_depth && data->md_image &&
			show_cursor(cursor);

    if (!(want_cursor || show_horiz_hair(cursor) || show_vert_hair(cursor)))
	goto Done;

    /*
    * Figure out where on screen to draw cursor.
    */
    shared->x = dtop->dt_rt_x - cursor->cur_xhot;
    shared->y = dtop->dt_rt_y - cursor->cur_yhot;

    /* Set plane group of plane group to write into */
    shared->plane_group = dtop->shared_info->mouse_plane_group;
    pr_set_planes(pr, shared->plane_group, full_planes);

    if (want_cursor) {
	if (show_cursor(cursor)) {
	    /*
	    * Prepare pixrect in which will store screen image.
	    */
	    win_lock_prepare_screen_pr(shared);

	    scrmpr_data = (struct mpr_data *)(shared->screen_pr.pr_data);
	    shared->screen_pr = *shape;
	    shared->screen_pr.pr_depth = pr->pr_depth;
	    shared->screen_pr.pr_data = (caddr_t) scrmpr_data;
	    *scrmpr_data = *data;
	    scrmpr_data->md_image = shared->screen_image;
	    scrmpr_data->md_linebytes = 
	        mpr_linebytes(shape->pr_width, pr->pr_depth);

	    /* Copy from display to memory */
	    if (pr_rop(&shared->screen_pr, 0, 0,
		       shared->screen_pr.pr_width, shared->screen_pr.pr_height,
		       PIX_SRC, pr, shared->x, shared->y))
		ROPERR(5)
        }
        if (show_enable_plane_cursor(dtop)) {
	    /* Prepare pixrect in which will store enable plane image */
	    dc->enable_mpr.pr_width = shape->pr_width;
	    dc->enable_mpr.pr_height = shape->pr_height;
	    ((struct mpr_data *)dc->enable_mpr.pr_data)->md_linebytes = 
	        mpr_linebytes(dc->enable_mpr.pr_width, 1);
	    /* Copy from display to memory */
	    pr_set_planes(pr, PIXPG_OVERLAY_ENABLE, full_planes);
	    if (pr_rop(&dc->enable_mpr, 0, 0, shape->pr_width, shape->pr_height,
	        PIX_SRC, pr, shared->x, shared->y))
		ROPERR(6)
	    pr_set_planes(pr, shared->plane_group, full_planes);
        }
    }

    /* prepair cross hair storage */
    /* get the horizontal & vertical info from the input window */
    dc->horiz_hair_mpr.pr_height = cursor->horiz_hair_thickness;
    dc->vert_hair_mpr.pr_width = cursor->vert_hair_thickness;

    dc->hair_x = dtop->dt_rt_x - (dc->vert_hair_mpr.pr_width / 2);
    dc->hair_y = dtop->dt_rt_y - (dc->horiz_hair_mpr.pr_height / 2);

    if (show_horiz_hair(cursor)) {
	/* construct the horizontal hair window/fullscreen rect */
	rect_construct(&hair_rect, 0,
		       fullscreen(cursor) ? dc->hair_y : 
			   dc->hair_y - cursorwin->w_screeny,
		       dc->horiz_hair_mpr.pr_width, 
		       dc->horiz_hair_mpr.pr_height);

	/* determine the intersection with the exposed area of
	 *  the window.
	 */
	dc->horiz_hair_rectlist = rl_null;
	if (fullscreen(cursor))
	    rl_initwithrect(&hair_rect, &dc->horiz_hair_rectlist);
	else {
	    rl_rectintersection(&hair_rect, &cursorwin->w_rlexposed,
				&dc->horiz_hair_rectlist);
	    /* convert to screen coordinates */
	    dc->horiz_hair_rectlist.rl_x = cursorwin->w_screenx;
	    dc->horiz_hair_rectlist.rl_y = cursorwin->w_screeny;
	    rl_normalize(&dc->horiz_hair_rectlist);
	    /* convert hair_rect top to screen coordinates */
	    hair_rect.r_top = dc->hair_y;
	}

	/* if length is not full, cut out excess */
	if (cursor->horiz_hair_length != CURSOR_TO_EDGE) {
	    int	max_length;
	    int	x_left;

	    if (horiz_border_gravity(cursor)) {
		max_length = fullscreen(cursor) ? dc->horiz_hair_mpr.pr_width :
		    cursorwin->w_rect.r_width;
		x_left = fullscreen(cursor) ? cursor->horiz_hair_length :
		    cursorwin->w_screenx + cursor->horiz_hair_length;

		hair_rect.r_left = x_left;
		hair_rect.r_width = max_length - 2 * cursor->horiz_hair_length;
		rl_rectdifference(&hair_rect, &dc->horiz_hair_rectlist,
				  &dc->horiz_hair_rectlist);
	    } else {
		/* length is from center */
		x_left = fullscreen(cursor) ? 0 : cursorwin->w_screenx;

		hair_rect.r_left = x_left;
		hair_rect.r_width =
		    dc->hair_x - cursor->horiz_hair_length - x_left;
		rl_rectdifference(&hair_rect, &dc->horiz_hair_rectlist,
				  &dc->horiz_hair_rectlist);

		hair_rect.r_left = dc->hair_x + cursor->horiz_hair_length;
		hair_rect.r_width = dc->horiz_hair_mpr.pr_width;
		rl_rectdifference(&hair_rect, &dc->horiz_hair_rectlist,
				  &dc->horiz_hair_rectlist);
	    }
	}

	if (cursor->horiz_hair_gap) {
	    /* cut out a gap the width of the cursor or as specified */
	    if (cursor->horiz_hair_gap == CURSOR_TO_EDGE) {
		hair_rect.r_width = shape->pr_width;
		hair_rect.r_left = shared->x;
	    } else {
		hair_rect.r_width = cursor->horiz_hair_gap;
		hair_rect.r_left = dc->hair_x - hair_rect.r_width / 2;
	    }
	    rl_rectdifference(&hair_rect, &dc->horiz_hair_rectlist,
			      &dc->horiz_hair_rectlist);
	}

	/* copy the image under the exposed area of the cross hair */
	if (!rl_empty(&dc->horiz_hair_rectlist)) {
	    if (dtop_rl_rop(&dc->horiz_hair_mpr, 0, dc->hair_y,
			    &dc->horiz_hair_rectlist,
			    PIX_SRC, pr, 0, 0)
		)
		ROPERR(7)
	}
    }

    if (show_vert_hair(cursor)) {
	/* construct the vertical hair window/fullscreen rect */
	rect_construct(&hair_rect, 
	    fullscreen(cursor) ? dc->hair_x : 
		 dc->hair_x - cursorwin->w_screenx, 
		       0,
	               dc->vert_hair_mpr.pr_width, 
		       dc->vert_hair_mpr.pr_height);

	/* determine the intersection with the exposed area of
	 *  the window.
	 */
	dc->vert_hair_rectlist = rl_null;
	if (fullscreen(cursor))
	    rl_initwithrect(&hair_rect, &dc->vert_hair_rectlist);
	else {
	    rl_rectintersection(&hair_rect, &cursorwin->w_rlexposed,
				&dc->vert_hair_rectlist);
	    /* convert to screen coordinates */
	    dc->vert_hair_rectlist.rl_x = cursorwin->w_screenx;
	    dc->vert_hair_rectlist.rl_y = cursorwin->w_screeny;
	    rl_normalize(&dc->vert_hair_rectlist);
	    /* convert hair_rect left to screen coordinates */
	    hair_rect.r_left = dc->hair_x;
	}

	/* if length is not full, cut out excess */
	if (cursor->vert_hair_length != CURSOR_TO_EDGE) {
	    int	max_length;
	    int	y_top;

	    if (vert_border_gravity(cursor)) {
		max_length = fullscreen(cursor) ? dc->vert_hair_mpr.pr_height :
		    cursorwin->w_rect.r_height;
		y_top = fullscreen(cursor) ? cursor->vert_hair_length :
		    cursorwin->w_screeny + cursor->vert_hair_length;

		hair_rect.r_top = y_top;
		hair_rect.r_height = max_length - 2 * cursor->vert_hair_length;
		rl_rectdifference(&hair_rect, &dc->vert_hair_rectlist,
				  &dc->vert_hair_rectlist);
	    } else {
		/* length is from center */
		y_top = fullscreen(cursor) ? 0 : cursorwin->w_screeny;

		hair_rect.r_top = y_top;
		hair_rect.r_height =
		    dc->hair_y - cursor->vert_hair_length - y_top;
		rl_rectdifference(&hair_rect, &dc->vert_hair_rectlist,
				  &dc->vert_hair_rectlist);

		hair_rect.r_top = dc->hair_y + cursor->vert_hair_length;
		hair_rect.r_height = dc->vert_hair_mpr.pr_height;
		rl_rectdifference(&hair_rect, &dc->vert_hair_rectlist,
				  &dc->vert_hair_rectlist);
	    }
	}

	if (cursor->vert_hair_gap) {
	    /* cut out a gap the height of the cursor or as specified */
	    if(cursor->vert_hair_gap == CURSOR_TO_EDGE) {
		hair_rect.r_height = shape->pr_height;
		hair_rect.r_top = shared->y;
	    } else {
		hair_rect.r_height = cursor->vert_hair_gap;
		hair_rect.r_top = dc->hair_y - hair_rect.r_height / 2;
	    }
	    rl_rectdifference(&hair_rect, &dc->vert_hair_rectlist,
			      &dc->vert_hair_rectlist);
	}

	/* copy the image under the exposed area of the cross hair */
	if (!rl_empty(&dc->vert_hair_rectlist)) {
	    if (dtop_rl_rop(&dc->vert_hair_mpr, dc->hair_x, 0,
			    &dc->vert_hair_rectlist,
			    PIX_SRC, pr, 0, 0)
	       )
		ROPERR(8)
	}
    }

    /*
    * Set planes such that will write to foreground and background
    * of current colormap segment.
    */
    planes = cursorwin->w_cms.cms_size - 1;
#define	planes_fully_implemented
#ifdef	planes_fully_implemented
    pr_putattributes(pr, &planes);
#else
    pr_set_planes(pr, shared->plane_group, planes);
#endif	planes_fully_implemented

    /*
    * Write to display
    */
    /* write the horizontal cross hair */
    if (show_horiz_hair(cursor) && !rl_empty(&dc->horiz_hair_rectlist)) {
	horiz_hair_set_up(dtop, TRUE);
	if (dtop_rl_rop(pr, 0, 0, &dc->horiz_hair_rectlist, 
		    cursor->horiz_hair_op | PIX_COLOR(cursor->horiz_hair_color),
		    (struct pixrect *)0, 0, 0))
	    ROPERR(9)
    }

    /* write the vertical cross hair */
    if (show_vert_hair(cursor) && !rl_empty(&dc->vert_hair_rectlist)) {
	vert_hair_set_up(dtop, TRUE);
	if (dtop_rl_rop(pr, 0, 0, &dc->vert_hair_rectlist, 
		    cursor->vert_hair_op | PIX_COLOR(cursor->vert_hair_color), 
		    (struct pixrect *)0, 0, 0))
	    ROPERR(10)
    }

    if (want_cursor) {
	int op;

        /* write the cursor */
	if (show_cursor(cursor)) {
	    cursor_set_up(dtop, TRUE);
	    if (pr_rop(pr, shared->x, shared->y,
		       shared->screen_pr.pr_width, shared->screen_pr.pr_height,
		       cursor->cur_function, shape, 0, 0))
		ROPERR(11)
	}
        /* write the enable plane cursor */
        if (show_enable_plane_cursor(dtop)) {
	    op = (dc->enable_color)? PIX_SRC | PIX_DST:
		PIX_NOT(PIX_SRC) & PIX_DST;
	    enable_plane_cursor_set_up(dtop, TRUE);
	    pr_set_planes(pr, PIXPG_OVERLAY_ENABLE, full_planes);
	    if (pr_rop(pr, shared->x, shared->y, dc->enable_mpr.pr_width,
		       dc->enable_mpr.pr_height, op, shape, 0, 0))
		    ROPERR(12)
	    pr_set_planes(pr, original_plane_group, full_planes);
        }
    }

Done:
    if (dtop->dt_dblcount) {
         if (pr_dbl_set(dtop->dt_pixrect, PR_DBL_READ, readstate, PR_DBL_WRITE,
             writestate, 0))
                 printf("Error in drawcursor: pr_dbl_set\n");
    }
    pr_set_planes(pr, original_plane_group, full_planes);
    return;

Roperr:
    pr_set_planes(pr, original_plane_group, full_planes);
    printf("Kernel cursor Roperr %D\n", rop_err_id);
}

/* rl should be normalized and in screen coordinates */
dtop_rl_rop(dest_pixrect, dest_dx, dest_dy, rl, op, src_pixrect,
	    src_dx, src_dy)
	struct pixrect	*dest_pixrect;
	int		dest_dx, dest_dy;
	struct rectlist	*rl;
	int		op;
	struct pixrect	*src_pixrect;
	int		src_dx, src_dy;
/* dtop_rl_rop does a pr_rop from src_pixrect to dst_pixrect for each
   area described in rl.
*/
{
    register struct rectnode	*rectnode;
    register struct rect	*r;

    for (rectnode = rl->rl_head; rectnode; 
         rectnode = rectnode->rn_next) {
        r = &rectnode->rn_rect;
	if (pr_rop(dest_pixrect, r->r_left - dest_dx, r->r_top - dest_dy, 
		   r->r_width, r->r_height, op, src_pixrect, 
		   r->r_left - src_dx, r->r_top - src_dy))
		return 1;
    }
    return 0;
} /* dtop_rl_rop */
   

/* return TRUE if the mutex, display or data locks are set. */
int
dtop_check_all_locks(lok_client)
	caddr_t	lok_client;
{
	Desktop			*dtop = (Desktop *) lok_client;
	register Win_lock_block	*shared_info = dtop->shared_info;

	return (shared_info->mutex.lock || shared_info->data.lock || 
		shared_info->display.lock);
}


static void
dtop_init_data_lock(dtop, process)
	register Desktop	*dtop;
	struct proc		*process;
{
	register Winlock	*wlock = &dtop->dt_datalock;

	wlok_setlock(wlock, &dtop->shared_info->data.lock, 1,
	    &wlock->lok_count_storage, process);
	wlock->lok_client = (caddr_t)dtop;
	wlock->lok_unlock_action = dtop_unlockdata;
	wlock->lok_timeout_action = dtop_timedout_data;
	wlock->lok_string = "data";
	wlock->lok_wakeup = (caddr_t) dtop->shared_info;
	wlock->lok_other_check = dtop_check_all_locks;
}

static void
dtop_init_display_lock(dtop, id, process)
	register Desktop	*dtop;
	int			id;
	struct proc		*process;
{
	register Winlock	*wlock = &dtop->dt_displaylock;

	wlok_setlock(wlock, &dtop->shared_info->display.lock, 1,
	    &dtop->shared_info->display.count, process);
	wlock->lok_client = (caddr_t) dtop;
	wlock->lok_unlock_action = dtop_unlockdisplay;
	wlock->lok_timeout_action = dtop_timedout_display;
	wlock->lok_string = "display";
	wlock->lok_wakeup = (caddr_t) dtop->shared_info;
	wlock->lok_id = id;
	wlock->lok_other_check = dtop_check_all_locks;
}

static void
dtop_init_mutex_lock(dtop, id, process)
	register Desktop	*dtop;
	int			id;
	struct proc		*process;
{
	register Winlock	*wlock = &dtop->dt_mutexlock;

	wlok_setlock(wlock, &dtop->shared_info->mutex.lock, 1,
	    &dtop->shared_info->mutex.count, process);
	wlock->lok_client = (caddr_t) dtop;
	wlock->lok_unlock_action = dtop_unlockmutex;
	/* use the display timedout routine */
	wlock->lok_timeout_action = dtop_timedout_display;
	wlock->lok_string = "mutex";
	wlock->lok_wakeup = (caddr_t) dtop->shared_info;
	wlock->lok_id = id;
	wlock->lok_other_check = dtop_check_all_locks;
}

void
dtop_putcolormap(dtop, cms_size, cmap)
	Desktop *dtop;
	int cms_size;
	struct cms_map *cmap;
{
	register struct pixrect *pr = dtop->dt_pixrect;
	int original_plane_group;

	/*
	 * Don't change colormap of monochrome
	 * because not really a colormap under device.  This code (in
	 * combination with the cursor tracking stuff) assumes
	 * that changing the colormap doesn't changed the bits.
	 * Removing test for colormap size leaves reverse cursor images
	 * whenever you change cursor windows with different backgrounds.
	 */
	if (dtop->dt_cmsize <= 2)
		return;
	/*
	 * Don't try to set colormap of a pixrect of depth one.
	 * If such a beast is encounted, change the plane group
	 * to a color plane group.
	 */
	original_plane_group = pr_get_plane_group(pr);
	if (pr->pr_depth == 1) {
		if (dtop->dt_plane_groups_available[PIXPG_8BIT_COLOR])
			pr_set_planes(pr, PIXPG_8BIT_COLOR, PIX_ALL_PLANES);
		else {
			if (!(dtop->dt_screen.scr_flags & SCR_OVERLAYONLY))
				printf("Tried to set 1 bit deep colormap\n");
			return;
		}
	}
	if (pr_putcolormap(pr, 0, cms_size,
	    cmap->cm_red, cmap->cm_green, cmap->cm_blue))
		printf("pr_putcolormap error\n");
	pr_set_planes(pr, original_plane_group, PIX_ALL_PLANES);
}



/*
* Pick the window with the cursor. If this window is not a double
* bufferer or cursor is not in any of the windows, choose a double
* bufferer from the list of windows (first one from the top to the
* the bottom ). There will always be atleast one double bufferer
* when this routine is called. Flip the display, write and read states.
*/

void
dtop_flip_display(dtop, w)
	register Desktop *dtop;
	register struct window *w;
{
	struct window *active_dbl = dtop->dt_curdbl;

	if (w == active_dbl) {
		/*
		* Flip the display and set write to old foreground, read to old
		* background.  Flip the dt_dbl_backgr and dt_dbl_frgnd field
		*/
		dtop_changedisplay(dtop,dtop->dt_dbl_bkgnd, dtop->dt_dbl_frgnd,
			FALSE);
		/*
		* If the cursor is not in the active double bufferer 
		* choose another double bufferer.  The same double
		* bufferer might get chosen again.
		*/
		if (dtop->dt_cursorwin != active_dbl ) {
			/* Choose a new double bufferer */
			dtop_choose_dblbuf(dtop);
			/*
			   If a new double bufferrer was chosen, copy from 
			   background buffer to forground buffer 
			*/
			if (active_dbl != dtop->dt_curdbl) 
				dtop_copy_dblbuffer(dtop, w);
		} 
	}
#if 0
	else {
		/* Pick a  window if current double bufferer is not active */
		if (curdbl process not active )
			dtop_choose_dblbuf(dtop);

	}
#endif
}


/*
* Choose a double bufferer 
*/
void
dtop_choose_dblbuf(dtop)
	register Desktop *dtop;
{
	register struct window *w = dtop->dt_cursorwin;

	dtop->dt_curdbl = WINDOW_NULL;
	if (dtop->dt_flags & DTF_DBLBUFFER ) {
		if ( w == WINDOW_NULL ) {
			wt_enumeratechildren(dtop_isdblwin, dtop->dt_rootwin, 
					(struct rect *)0);
			dtop->dt_flags &= ~DTF_DBL_FOUND;
		}
		else if ( !(w->w_flags & WF_DBLBUF_ACCESS ) ) {
			wt_enumeratechildren(dtop_isdblwin, dtop->dt_rootwin, 
					(struct rect *)0);
			dtop->dt_flags &= ~DTF_DBL_FOUND;
		}
		else dtop->dt_curdbl = w;
	}
}

/*
* This rountine is called by wt_enumeratechildren for each of the
* windows determines whether the window is a double bufferer or not.
* It sets a flag DTF_DBL_FOUND if a double bufferer is found.  The idea
* is to find the first double bufferer (from TOP to BOTTOM ).
*/

/*ARGSUSED*/
int
dtop_isdblwin(w, rect)
	register struct window *w;
	struct rect   *rect;
{
	if  ( !(w->w_desktop->dt_flags & DTF_DBL_FOUND ) )
		if ( w->w_flags & WF_DBLBUF_ACCESS )
		{
			w->w_desktop->dt_flags |= DTF_DBL_FOUND;
			w->w_desktop->dt_curdbl = w;
		}
}



/*
* Remove the cursor, flip the display and redraw the
* cursor in the foreground
*/
void
dtop_changedisplay(dtop, fore, back, first)
	Desktop *dtop;
	int 	fore, back;
	int	first;	/* Is it called for the first time ? */
{
	(void)dtop_cursordraw(dtop);
	if (first)
		dtop->dt_dblcount++;
	if (dtop) {
		dtop->dt_dbl_frgnd = fore;
		dtop->dt_dbl_bkgnd = back;
	}
	if (pr_dbl_set(dtop->dt_pixrect, PR_DBL_DISPLAY,dtop->dt_dbl_frgnd, 0))
		printf("Error in dtop_changedisplay \n");
		
}


/*
* Draw cursor if no lock mutex
*/

int 
dtop_cursordraw(dtop) 
Desktop *dtop;
{
      if ( cursor_up(dtop) || horiz_hair_up(dtop) || vert_hair_up(dtop) ) 
          if (!win_lock_mutex_locked(dtop->shared_info)) {
              dtop_drawcursor(dtop);
              return(1);
      }
      return(0);
}

 
/*
* copy foreground buffer into background
* buffer
*/
 
 
void
dtop_copy_dblbuffer(dtop, w)
register Desktop *dtop;
register struct window *w;
{
       int cursor_removed;
 
       cursor_removed = dtop_cursordraw(dtop);
       /* Copy foreground buffer into bkgnd buffer */
       if (pr_dbl_set(dtop->dt_pixrect, PR_DBL_READ, dtop->dt_dbl_frgnd, 
           PR_DBL_WRITE, dtop->dt_dbl_bkgnd, 0))
              printf("Error dbl_set flip disp #2\n");
       /* Screen x ,y need to be normalized */
       if (dtop_rl_rop(dtop->dt_pixrect,-w->w_screenx, -w->w_screeny, 
           &w->w_rlexposed, PIX_SRC, dtop->dt_pixrect,-w->w_screenx,
           -w->w_screeny))
               printf("roperr in flip display\n");
       if (cursor_removed)
               (void)dtop_cursordraw(dtop);
}

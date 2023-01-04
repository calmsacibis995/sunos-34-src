#ifndef lint
static	char sccsid[] = "@(#)winshared.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * SunWindows Desktop shared memory update module.
 */

#include "../sunwindowdev/wintree.h"
#include "../sunwindow/cursor_impl.h"	/* for cursor access macros */
#include "../pixrect/pr_util.h"		/* for mpr support */
#include "../pixrect/pr_planegroups.h"	/* for plane groups */

void
win_shared_update_cursor(dtop)
	register Desktop	*dtop;
{
	Win_lock_block		*shared_info = dtop->shared_info;
	register Window		*cursorwin = dtop->dt_cursorwin;
	register Win_shared_cursor *shared_cursor = &shared_info->cursor_info;

	if (win_lock_mutex_locked(shared_info)) {
		shared_info->status.update_cursor = TRUE;
		return;
	}

	if (!cursorwin) {
		/* no cursor window, so zero the cursor info */
		shared_cursor->cursor.flags = 0;
		shared_cursor->pr.pr_width = shared_cursor->pr.pr_height = 0;
		return;
	} else {
		shared_cursor->cursor = cursorwin->w_cursor;
		shared_cursor->pr = cursorwin->w_cursormpr;
		shared_cursor->pr_data = cursorwin->w_cursordata;
		bcopy((caddr_t)cursorwin->w_cursorimage,
		    (caddr_t)shared_cursor->image, CUR_MAXIMAGEBYTES);
	}
	
	shared_info->mouse_plane_group = (!dtop->dt_cursorwin)?
	    PIXPG_CURRENT: dtop->dt_cursorwin->w_plane_group;
}


void
win_shared_update_cursor_active(shared_info, active)
	Win_lock_block		*shared_info;
	int			active;
{
	if (win_lock_mutex_locked(shared_info)) {
		shared_info->status.update_cursor_active = TRUE;
		return;
	}

	shared_info->cursor_info.cursor_active = active;
}


void
win_shared_update_mouse_xy(shared_info, x, y)
	Win_lock_block	*shared_info;
	int		x, y;
{
	if (win_lock_mutex_locked(shared_info)) {
		shared_info->status.update_mouse_xy = TRUE;
		return;
	}

	shared_info->mouse_x = x;
	shared_info->mouse_y = y;
}


void
win_shared_update(dtop)
	register Desktop	*dtop;
{
	register Win_lock_block		*shared_info = dtop->shared_info;
	register Win_lock_status	*status = &shared_info->status;

	if (win_lock_mutex_locked(shared_info))
		return;

	if (status->update_cursor) {
		if (dtop->dt_cursorwin)
			win_shared_update_cursor(dtop);
		status->update_cursor = FALSE;
	}

	if (status->update_cursor_active) {
		win_shared_update_cursor_active(shared_info, 
		    dtop->dt_ws->ws_loc_dtop == dtop);
		status->update_cursor_active = FALSE;
	}

	if (status->update_mouse_xy) {
		win_shared_update_mouse_xy(shared_info, dtop->dt_rt_x, 
		    dtop->dt_rt_y);
		status->update_mouse_xy = FALSE;
	}
}

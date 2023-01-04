#ifndef lint
static	char sccsid[] = "@(#)wmgr_cursors.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Window manager cursors.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_cursor.h>

/*
 *	cursor displayed while waiting for a prompt to get a response
 */

short	confirm_cursor_image[16] =  {
#		include	<images/confirm.cursor>
};
mpr_static(confirm_cursor_mpr, 16, 16, 1, confirm_cursor_image);
struct cursor	confirm_cursor = { 8, 8, PIX_SRC, &confirm_cursor_mpr};

/*
 *	3 cursors for Move
 */

short	move_cursorimage[16] = {
#		include <images/move.cursor>
};
mpr_static(move_mpr, 16, 16, 1, move_cursorimage);
struct	cursor wmgr_move_cursor = { 6, 7, PIX_SRC, &move_mpr};

short	move_h_cursorimage[16] = {
#		include <images/move_h.cursor>
};
mpr_static(move_h_mpr, 16, 13, 1, move_h_cursorimage);
struct	cursor wmgr_moveH_cursor = { 7, 6, PIX_SRC, &move_h_mpr};

short	move_v_cursorimage[16] = {
#		include <images/move_v.cursor>
};
mpr_static(move_v_mpr, 13, 16, 1, move_v_cursorimage);
struct	cursor wmgr_moveV_cursor = { 7, 7, PIX_SRC, &move_v_mpr};

/*	cursors for use during a Stretch operation
 *	-- shape depends on part being moved
 *
 *	first a null one for the center:
 */

short	stretchMID_cursorimage[1] = { 0 };
mpr_static(stretchMID_mpr, 0, 0, 1, stretchMID_cursorimage);
struct	cursor wmgr_stretchMID_cursor = { 0, 0, PIX_SRC, &stretchMID_mpr};

/*
 *	each pair opposite sides share a single image
 */

short	stretch_v_cursorimage[16] = {
#		include <images/stretch_v.cursor>
};
mpr_static(stretch_v_mpr, 8, 16, 1, stretch_v_cursorimage);

struct	cursor wmgr_stretchE_cursor = { 4, 8, PIX_SRC, &stretch_v_mpr};

struct	cursor wmgr_stretchW_cursor = { 3, 8, PIX_SRC, &stretch_v_mpr};


short	stretch_h_cursorimage[16] = {
#		include <images/stretch_h.cursor>
};
mpr_static(stretch_h_mpr, 16, 8, 1, stretch_h_cursorimage);

struct	cursor wmgr_stretchN_cursor = { 8, 3, PIX_SRC, &stretch_h_mpr};

struct	cursor wmgr_stretchS_cursor = { 8, 4, PIX_SRC, &stretch_h_mpr};



/*
 *	corners are treated individually
 */


short	stretchNW_cursorimage[16] = {
#		include <images/stretchNW.cursor>
};
mpr_static(stretchNW_mpr, 16, 16, 1, stretchNW_cursorimage);
struct	cursor wmgr_stretchNW_cursor = { 3, 3, PIX_SRC, &stretchNW_mpr};

short	stretchNE_cursorimage[16] = {
#		include <images/stretchNE.cursor>
};
mpr_static(stretchNE_mpr, 16, 16, 1, stretchNE_cursorimage);
struct	cursor wmgr_stretchNE_cursor = { 13, 3, PIX_SRC, &stretchNE_mpr};

short	stretchSE_cursorimage[16] = {
#		include <images/stretchSE.cursor>
};				
mpr_static(stretchSE_mpr, 16, 16, 1, stretchSE_cursorimage);
struct	cursor wmgr_stretchSE_cursor = { 13, 13, PIX_SRC, &stretchSE_mpr};

short	stretchSW_cursorimage[16] = {
#		include <images/stretchSW.cursor>
};				
mpr_static(stretchSW_mpr, 16, 16, 1, stretchSW_cursorimage);
struct	cursor wmgr_stretchSW_cursor = { 3, 13, PIX_SRC, &stretchSW_mpr};

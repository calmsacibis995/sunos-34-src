#ifndef lint
static	char sccsid[] = "@(#)gpbuf.c 1.6 87/04/14 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/***********
 *
 *
 *		 @@@@  @@@@   @@@@   @   @  @@@@@
 *		@      @   @  @   @  @   @  @
 *		@   @  @@@@   @@@@   @   @  @@@@
 *		@   @  @      @   @  @   @  @
 *		 @@@@  @      @@@@    @@@   @
 *
 *
 * GP BUFFER  ROUTINES
 * Create_GP(vt, ct, pw, n)	create new GP buffer of N blocks
 *
 * destroy_gp(gp)		destroy GP buffer
 * flush_gp(gp)			post GP buffer commands, flush buffer
 * clear_gp(gp, c)		clear GP screen
 * oval_gp(gp,dim)		draw an oval
 * box_gp(gp,dim)		draw a box
 * print_gp(gp)			print command buffer
 * close_gp(gp)			close off current object (polygon)
 * get_gp(gp, a, v)		get GP attribute
 * set_gp(gp, a, v)		set GP attribute
 *
 * xform_X(gpb, p)		transform coordinate
 * move_X(gp, p)		move absolute vector/polygon
 * draw_X(gp, p)		draw absolute vector/polygon
 * mrel_X(gp, p)		move relative vector/polygon
 * drel_X(gp, p)		draw relative vector/polygon
 *
 * X = { vec_i2d, vec_i3d, vec_f2d, vec_f3d,
 *	 poly_i2d, poly_i3d, poly_f2d, poly_f3d }
 *
 * gp_winview(gp)		map viewport coordinates to window
 * gp_view_xform(gp)		calculate viewport -> GP mapping
 * gp_win_xform(gp)		calculate GP -> window mapping
 * gp_init(gpb)			initialize GP attributes
 * gp_setview(gpb)		set internal viewing coordinates
 * gp_setclip(gpb)		set internal clipping coordinates
 * gp_winclip(gpb)		load window clip list
 * gp_allocbuf(gpb)		allocate new GP buffer
 * gp_start(gpb)		start vector/polygon list
 * gp_poly_bound(gpb)		establish new polygon bound
 *
 *
 **********/
#include "gpbuf_int.h"
#include <stdio.h>
#include <sun/gpio.h>
#include <pixrect/gp1var.h>
#include "bitmap.h"

/*
 * GP debugging flag
 * If set and CALLDEBUG/DEBUG switch on, GP debugging messages are printed
 */
	int	vpdbg;				/* debug flag */

	MATRIX	Identity3D = {			/* 3D identitity matrix */
	1.0,	0.0,	0.0,	0.0,
	0.0,	1.0,	0.0,	0.0,
	0.0,	0.0,	1.0,	0.0,
	0.0,	0.0,	0.0,	1.0 };

/*
 * This table contains the GP1 opcodes for starting vectors or polygons
 */
static	short	startypes[] = {
	GP1_XF_LINE_INT_2D, GP1_XF_LINE_INT_3D,	/* 2D/3D integer vector */
	GP1_XF_LINE_FLT_2D, GP1_XF_LINE_FLT_3D,	/* 2D/3D floating vector */
	GP1_XF_PGON_INT_2D, GP1_XF_PGON_INT_3D,	/* 2D/3D integer polygon */
	GP1_XF_PGON_FLT_2D, GP1_XF_PGON_FLT_3D,	/* 2D/3D floating polygon */
	};

/****
 *
 * Create_GP(gpb, t, pw, n)
 * Create new GP buffer. The GP is initialized and a status block is allocated.
 * A buffer of N blocks for GP commands is allocated from shared memory.
 *
 * returns:
 *   -> GPBUF structure created/updated
 *   NULL on error
 *
 * notes:
 * Initial checks
 * 1. Argument validity: non-null pixwin, positive size
 * 2. In a GP window?
 * 3. Can allocate a static block?
 * 4. Can allocate GP buffer structure?
 * 5. Initialize new GPBUF structure:
 *	gpb_gfd		file descriptor of GP device
 *	gpb_size	GP command buffer size in blocks
 *	gpb_coord	GP coordinate type
 *	gpb_win		window handle (pixwin)
 *	gpb_mindev	true minor device number
 *	gpb_shmem	GP shared memory
 *	gpb_clipid	clip buffer ID
 *	gpb_sbindex	index of GP status block
 *
 * Each GP buffer is associated with a static block that contains the GP1
 * context. GP1 command buffers start with a USE_CONTEXT command that
 * references this static block.
 *
 ****/
GPBUF
Create_GP(vt, ct, pw, n)
/*    --needs--			 */
	int	vt, ct;		/* type */
FAST	struct pixwin *pw;	/* window handle */
	int	n;		/*  buffer size */
{
/*    --calls--			 */
extern	OBJID	NewObj();	/* OBJ allocate object */
local	void	gp_init();	/* initialize GP */
local	void	gp_allocbuf();	/* allocate GP buffer */
extern	int	gp1_rop();

/*    --uses--			 */
local	_vpfp	*vpftab[];	/* -> GP function dispatch tables */
FAST	GPDATA	*gpb;		/* -> GP buffer structure */
	GPBUF	gpbobj;

	if (pw == NULL) error("Create_VP: NULL pixwin argument\n", 0, 0);
	if (n == 0) n = GPB_DEFAULTSIZE;
	if (pw->pw_pixrect->pr_ops->pro_rop != gp1_rop)
	   error("Create_VP: cannot make viewport of type VP_GP - system has no GP\n", 0, NO);
	if (IsNull(gpbobj = NewObj(T_IOBJ, sizeof(GPDATA))))
	   error("Create_VP: cannot allocate GPBUF context\n", 0, 0);
	gpb = ObjAddr(gpbobj, GPDATA);
	gpb->gpb_gfd = gp1_d(pw->pw_clipdata->pwcd_prmulti)->ioctl_fd;
	gpb->gpb_mindev = gp1_d(pw->pw_clipdata->pwcd_prmulti)->minordev;
	if ((gpb->gpb_sbindex = gp1_get_static_block(gpb->gpb_gfd)) < 0)
	   error("Create_VP: cannot acquire GP static block\n", 0, 0);
	if (n <= 0) n = 1;
	gpb->gpb_vp.vp_win = pw;		/* and pixwin handle */
	gpb->gpb_size = n;			/* save the size */
	gpb->gpb_type = VP_GP1;			/* we are a GP viewport */
	gpb->gpb_coord = gpb->gpb_ftype =	/* save coordinate type */
		ct & VP_COORDMASK;
	gpb->gpb_vp.vp_func = vpftab[ct];	/* function table */
	gpb->gpb_shmem = 			/* shared memory */
	   gp1_d(pw->pw_clipdata->pwcd_prmulti)->gp_shmem;
	gpb->gpb_visible = 1;			/* vectors visible */
	gpb->gpb_clipid = 0;			/* no clip list */
#ifdef	CALLDEBUG
	if (vpdbg) printf("Create_VP: gpb = %X, size = %d\n", gpb, ct);
#endif
	gp_allocbuf(gpb);			/* allocate buffer */
	gp_init(gpb);				/* initialize */
	return (gpbobj);
}

/****
 *
 * wait_gp(gpobj)
 * Wait for GP1 to finish executing all command buffers.
 *
 ****/
wait_gp(gpbobj)
/*    --needs--			 */
	GPBUF	gpbobj;
{
FAST	GPDATA	*gpb;

	gpb = ObjAddr(gpbobj, GPDATA);
	if (gp_flush(gpb))
           gp1_sync(gpb->gpb_shmem, gpb->gpb_gfd);
}

/****
 *
 * mapcolor_gp(gpobj)
 * Map user color to physical color.
 *
 * returns:
 *   physical color
 *
 * notes:
 * This routine is a kludge put in because certain SunWindows related
 * functions need to know the exact physical color to work.
 *
 ****/
mapcolor_gp(gpbobj, c)
/*    --needs--			 */
	GPBUF	gpbobj;
	int	c;
{
FAST	VPDATA	*gpb;

	return(vp_addr(gpbobj)->vp_cindex[c]);
}

/****
 *
 * flush_gp(gpbobj)
 * Flush GP buffer, posting any commands.
 *
 * notes:
 * The general philosophy below is to check to see if the window has
 * changed since last post before posting the current full buffer.
 * We trust the window system to reliably change the clip ID to indicate
 * a window change. Most of the time this works, but the scheme is not
 * foolproof.
 *
 * 1. Append EOCL command causing GP to free allocated
 *    buffer after it is finished executing the commands
 * 2. Check for window damage
 *    if clip ID has changed, allocate new buffer and reload clip list
 *    (allocate new buffer to reload clip list)
 * 3. Post GP buffer
 * 4. Allocate new buffer (same size as original)
 *
 * There is a major hole in this method: SET_VWP commands (used to map
 * from GP space to physical screen coordinates) may be embedded in the
 * buffer to be posted. These commands use the physical position of the
 * window on the screen. If that has changed between the time the SET_VWP
 * command was put into the buffer and the time the buffer is posted,
 * the GP may draw incorrectly. One way to fix this would be to flush
 * the buffer after each SET_VWP command. This would really degrade
 * performance for key applications which use this package, so we left
 * the bug as it is. If you are doing double buffering or using
 * notify_dispatch (rather than window_main_loop) you will probably not
 * notice this bug. If you move the window while the GP is drawing but
 * do not repaint shortly afterwards, you might notice the GP draw some
 * stuff in the wrong place. Isn't buffering wonderful?
 *
 ****/
flush_gp(gpbobj) GPBUF gpbobj; { gp_flush(vp_addr(gpbobj)); }

gp_flush(gpb)
/*    --needs--			 */
FAST	GPDATA	*gpb;
{
/*    --calls--			 */
local	void	gp_winview();	/* set window viewing transform */
local	void	gp_winclip();	/* download clip list */
local	void	gp_allocbuf();	/* allocate new GP buffer */

/*    --uses--			 */
FAST	struct pixwin *pw;	/* -> GP pixwin */
	int	oldofs, id;	/* old buffer offset, new clip id */

/*
 * End current block.
 * 1) Clear matrix update flags
 * 2) If the block is empty, don't bother flushing, just exit
 * 3) Mark any vector/polygon lists as closed
 * 4) Append EOCL / free buffer command
 */
	gpb->gpb_matupd = 0;
	if ((gpb->gpb_ptr - 1) <= &((short *) gpb->gpb_shmem)[gpb->gpb_cmdofs])
	   return (NO);
#ifdef	CALLDEBUG
	if (vpdbg) printf("flush_gp: %d words used\n",
		gpb->gpb_ptr - &((short *) gpb->gpb_shmem)[gpb->gpb_cmdofs]);
#endif
	gpb->gpb_vstart = 0;			/* end vector list */
	PUT_CMD(gpb->gpb_ptr, GP1_EOCL, GP1_FREEBLKS);
	PUT_INT(gpb->gpb_ptr, gpb->gpb_cmdbv);
	oldofs = gpb->gpb_cmdofs;		/* save buffer offset */
#ifdef	DEBUG
	if (vpdbg) gp_print(gpb);
#endif
/*
 * A new buffer is allocated to load the clip list. The pixwin layer
 * changes the pixwin clip id (pwcd_clipid) if the window has moved,
 * changed size or become exposed. We cannot post the full buffer we have
 * until we reload the new clip list. The order of the operations below
 * is VERY SIGNIFICANT. If you change it, prepare to spend a week or
 * more tracking down strange side effects. Whatever you do, don't call
 * me about it.
 */
	pw = gpb->gpb_vp.vp_win;		/* -> GP window */
relock:
	pw_lock(pw, &(gpb->gpb_vp.vp_wdim));
	if (gpb->gpb_clipid !=			/* clip list changed? */
	    (id = pw->pw_clipdata->pwcd_clipid))
	  {
	   gp_allocbuf(gpb);			/* allocate clip buffer */
	   gpb->gpb_flags |= GPB_RESIZE_FLAG;	/* set window resize flag */
	   gp_winclip(gpb);			/* load clip list */
	   gpb->gpb_ptr = &((short *) gpb->gpb_shmem)[gpb->gpb_cmdofs] + 1;
	   gpb->gpb_clipid = id;		/* save new clip ID */
	   pw_unlock(pw);			/* unlock old window size */
	   goto relock;				/* lock with new window size */
	  }
	POSTGPBUF(gpb, oldofs);			/* post our buffer */
	pw_unlock(pw);				/* unlock window */
	gp_allocbuf(gpb);			/* allocate new buffer */
	return (YES);
}


/****
 *
 * close_gp(gpbobj)
 * Close current polygon. Does not flush buffer.
 *
 * notes:
 * Polygon open-ness is defined by gpb_vstart field. If a vector or
 * polygon list is under construction (i.e. the last command was a move or
 * draw) gpb_vstart contains the GP1 opcode for the vector/polygon command.
 * Otherwise, it is zero. All MOVE and DRAW commands check gpb_vstart
 * to see if they have to start a new list or not.
 *
 ****/
static
close_gp(gpbobj)
/*    --needs--			 */
FAST	GPBUF	gpbobj;
{
	ObjAddr(gpbobj, GPDATA)->gpb_vstart = 0;
}

/****
 *
 * repaint_gp(gpbobj)
 * Repaint the contents of the window.
 * For future expansion - not used now.
 *
 ****/
static
repaint_gp() { return; }


/****
 *
 * clear_gp(gpbobj, color)
 * Clears the entire GP window to the given color. The ENTIRE WINDOW is
 * cleared, even if the clip area is set to a rectangle which does not
 * cover the window.
 *
 * notes:
 * To clear the window, the GP1_PR_ROP_NF command is used. This command is
 * designed to not require a static block so all of the operands
 * must be specified. It also does not clip to the clip list (which is
 * in the static block) so it must be performed for each rectangle in
 * the clip list.
 *
 * The coordinates of the box are given with respect to the window.
 * The coordinates of each clip list rectangle are with respect to
 * the entire screen (device coordinates). PRROPNF wants the box coordinates
 * to be with respect to the clip rectangle.
 *
 * It has been observed that the GP1_PR_xxx commands cannot be mixed
 * with other commands in the same buffer. Therefore, we flush the buffer
 * before and after the clear. If this is ever fixed, we can remove
 * the redundant flushes.
 *
 * Whenever the screen is cleared, the Z buffer is also cleared (if
 * VP_HIDDENSURF is enabled)
 *
 ****/
static
clear_gp(gpbobj, color)
/*    --needs--                  */
	GPBUF   gpbobj;
        int     color;
{
/*    --calls--			 */
local	void	gp_winview();	/* set window viewing transform */
local	void	gp_clearzbuf();	/* clear Z buffer */

/*    --uses--			 */
FAST    GPDATA   *gpb;
FAST	struct pixwin_prlist *prl;
FAST	PTR	ptr;
FAST	struct gp1pr *dmd;
FAST	int	rop, fbi;

	gpb = ObjAddr(gpbobj, GPDATA);
	gpb->gpb_vstart = 0;			/* close vector/polygon */
	if (gpb->gpb_flags) gp_winview(gpb);	/* recalculate viewing? */
	gp_flush(gpb);				/* GP1 bug */
	dmd = gp1_d(gpb->gpb_vp.vp_win->pw_clipdata->pwcd_prmulti);
	fbi = dmd->cg2_index;			/* save FB index, color, rop */
	rop = PIX_SRC | PIX_COLOR(gpb->gpb_cindex[color & gpb->gpb_pixmask]);
	prl = gpb->gpb_vp.vp_win->pw_clipdata->pwcd_prl;
	while (prl)				/* for each clip list rect */
	  {
	   MAKEROOM(11 * sizeof(short));
	   ptr.sh = gpb->gpb_ptr;		/* start over again */
	   PUT_CMD(ptr.sh, GP1_PR_ROP_NF, gpb->gpb_fbplanes & 0XFF);
	   PUT_SHORT(ptr.sh, fbi); PUT_SHORT(ptr.sh, rop);
	   dmd = gp1_d(prl->prl_pixrect);	/* clip pixrect */
	   PUT_SHORT(ptr.sh, dmd->cgpr_offset.x);	/* X, Y origin of rect */
	   PUT_SHORT(ptr.sh, dmd->cgpr_offset.y);
	   PUT_SHORT(ptr.sh, prl->prl_pixrect->pr_size.x); /* X, Y size of rect */
	   PUT_SHORT(ptr.sh, prl->prl_pixrect->pr_size.y);
	   PUT_SHORT(ptr.sh, 0);			/* w/r to clip pixrect */
	   PUT_SHORT(ptr.sh, 0);
	   PUT_SHORT(ptr.sh, prl->prl_pixrect->pr_size.x);
	   PUT_SHORT(ptr.sh, prl->prl_pixrect->pr_size.y);
	   gpb->gpb_ptr = ptr.sh;
	   prl = prl->prl_next;			/* go to next one */
	  }
	gp_clearzbuf(gpb);			/* clear Z buffer */
	gp_flush(gpb);				/* GP1 bug */
}

/****
 *
 * destroy_gp(gpbobj)
 * Free GP block and associated buffer.
 *
 * notes:
 * 1) Append EOCL to buffer and post it (causing GP to free when done)
 * 2) free polygon pattern table
 * 3) free vector texture table
 * This pattern and texture table stuff is really bogus. Hopefully
 * it will go away soon and be replaced by something that makes sense.
 *
 ****/
static
destroy_gp(gpbobj)
/*    --needs--			 */
	GPBUF	gpbobj;
{
/*    --uses--			 */
extern	void	Destroy_Bitmap();
FAST	GPDATA	*gpb;
FAST	short	*ptr;

	gpb = ObjAddr(gpbobj, GPDATA);
	ptr = &((short *) gpb->gpb_shmem)[gpb->gpb_cmdofs];
	++ptr;					/* skip USEFRAME */
	PUT_CMD(ptr, GP1_EOCL, GP1_FREEBLKS);	/* free command buffer */
	PUT_INT(ptr, gpb->gpb_cmdbv);
	POSTGPBUF(gpb, gpb->gpb_cmdofs);
	gp1_free_static_block(gpb->gpb_gfd, gpb->gpb_sbindex);
	DelObj(gpb->gpb_textab, NULL);		/* free texture table */
	DelObj(gpb->gpb_pattab, Destroy_Bitmap);/* free pattern table */
	FreeObj(gpbobj);			/* free GPBUF info */
}

/****
 *
 * drel_poly_i2d(gpbobj, p)
 * Draw relative polygon 2D.
 *
 ****/
static
drel_poly_i2d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	draw_poly_i2d();

/*    --uses--			 */
	POINT	pen;

	pen = *p;
	pen.i.x += Get_VP(gpbobj, VP_PEN)->i.x;
	pen.i.y += Get_VP(gpbobj, VP_PEN)->i.y;
	draw_poly_i2d(gpbobj, &pen);
}

/****
 *
 * draw_poly_i2d(gpbobj, p)
 * Append integer 2D polygon edge to GP buffer. Polygon list info:
 *	gpb_ptr		-> next available spot in buffer
 *	gpb_left	# of bytes left in buffer
 *	gpb_pen		current position
 *	gpb_vecsize	-> vector list size word
 *	gpb_polysize	-> polygon list size word
 *	gpb_vstart	vector/polygon list start code
 *
 * notes:
 * Polygons are always started at the top of a buffer. If a polygon edge
 * does not fit in the buffer, the polygon list is ended the buffer is
 * flushed and a new polygon is started. The new polygon and the old one
 * share a common edge (from the first polygon vertex to the last). This
 * method is not general - it only works for convex polygons, but it is
 * better than nothing.
 *
 * The GP1 has a bug which causes buffers full of large polygons to affect
 * other graphics on the screen. If this is ever fixed, the polygon scheme
 * might have to be changed. No attempt is made to shuffle polygon edges
 * to a new buffer if the current buffer is overflowed. This might be a
 * good idea in the future (as long as I don't have to do it).
 *
 ****/
static
draw_poly_i2d(gpbobj, p)
/*    --needs--			 */
	GPBUF	gpbobj;		/* -> GP buffer */
	POINT	*p;		/* point to add */
{
local	void	gp_start();	/* start new polygon list */

FAST	GPDATA	*gpb;
FAST	short	*ptr;		/* shared memory pointer */
	POINT	temp;		/* temporary pen position */

	gpb = ObjAddr(gpbobj, GPDATA);
	if (gpb->gpb_vstart == 0)		/* no polygon started? */
	  {
	   gp_start(gpb);			/* start new polygon */
	   gpb->gpb_poly = gpb->gpb_pen;	/* save start of polygon */
	   draw_poly_i2d(gpbobj, &(gpb->gpb_pen)); /* initialize 1st edge */
	  }
	if (!ROOMFOR(2 * sizeof(int)))		/* will new point fit? */
	   {
	    gp_start(gpb);			/* start new polygon list */
	    temp = gpb->gpb_pen;		/* save pen position */
	    draw_poly_i2d(gpb, &(gpb->gpb_poly));
	    draw_poly_i2d(gpb, &temp);		/* store common edge */
	   }
	ptr = gpb->gpb_ptr;			/* where to put vector */
	PUT_INT(ptr, gpb->gpb_pen.i.x = p->i.x); /* save new edge */
	PUT_INT(ptr, gpb->gpb_pen.i.y = p->i.y); /* save new edge */
	USEUP(2 * sizeof(int));
	++(*(gpb->gpb_vecsize));		/* update vector list size */
	gpb->gpb_ptr = ptr;			/* update pointer */
}

/****
 *
 * drel_vec_i2d(gpbobj, p)
 * Draw relative vector 2D.
 *
 ****/
static
drel_vec_i2d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	draw_vec_i2d();

/*    --uses--			 */
	POINT	pen;

	pen = *p;
	pen.i.x += Get_VP(gpbobj, VP_PEN)->i.x;
	pen.i.y += Get_VP(gpbobj, VP_PEN)->i.y;
	draw_vec_i2d(gpbobj, &pen);
}

/****
 *
 * draw_vec_i2d(gpbobj, p)
 * Append 2D integer vector to GP buffer. Vectors list info:
 *	gpb_ptr		-> next available spot in buffer
 *	gpb_left	# of bytes left in buffer
 *	gpb_pen		current position
 *	gpb_vstart	vector list start code
 *
 * notes:
 * If the vector will not fit in the buffer, the current commands are
 * posted and the vector list starts a new buffer. Each vector has a
 * move/draw flag associated with it. The end of the vector list is
 * also marked with a flag. Why can't we just have GP1_MOVE and GP1_DRAW
 * instead of flag city?
 *
 ****/
static
draw_vec_i2d(gpbobj, p)
/*    --needs--			 */
	GPBUF	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	void	gp_start();	/* start new vector/polygon list */

/*    --uses--			 */
FAST	GPDATA	*gpb;
FAST	PTR	ptr;		/* shared memory pointer */

	gpb = ObjAddr(gpbobj, GPDATA);
	if ((gpb->gpb_vstart == 0) ||		/* no list started? */
	    !ROOMFOR(2 * sizeof(int) + sizeof(short)))
	  {
	   gpb->gpb_vstart = 0;
	   move_vec_i2d(gpbobj, &(gpb->gpb_pen));
	  }
	ptr.sh = gpb->gpb_ptr;			/* where to put vector */
	*(ptr.sh - (2 * sizeof(int)) / sizeof(short) - 1) &= ~GPB_END_FLAG;
	PUT_SHORT(ptr.sh, GPB_END_FLAG | GPB_DRAW_FLAG);
	PUT_INT(ptr.in, gpb->gpb_pen.i.x = p->i.x);
	PUT_INT(ptr.in, gpb->gpb_pen.i.y = p->i.y);
	USEUP(2 * sizeof(int) + sizeof(short));
	gpb->gpb_ptr = ptr.sh;			/* update pointer */
}

/****
 *
 * move_vec_i2d(gpbobj, p)
 * Append 2D integer vector to GP buffer. Vectors list info:
 *
 ****/
static
move_vec_i2d(gpbobj, p)
/*    --needs--			 */
	GPBUF	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	void	gp_start();	/* start new vector/polygon list */

/*    --uses--			 */
FAST	GPDATA	*gpb;
FAST	PTR	ptr;		/* shared memory pointer */

	gpb = ObjAddr(gpbobj, GPDATA);
	if ((gpb->gpb_vstart == 0) ||		/* no list started? */
	    !ROOMFOR(2 * sizeof(int) + sizeof(short)))
	  {
	   gp_start(gpb);			/* start new vector list */
	   ptr.sh = gpb->gpb_ptr;		/* where to put vector */
	  }
	else
	  {
	   ptr.sh = gpb->gpb_ptr;		/* where to put vector */
	   *(ptr.sh - (2 * sizeof(int)) / sizeof(short) - 1) &= ~GPB_END_FLAG;
	  }
	PUT_SHORT(ptr.sh, GPB_END_FLAG | GPB_MOVE_FLAG);
	PUT_INT(ptr.in, gpb->gpb_pen.i.x = p->i.x);
	PUT_INT(ptr.in, gpb->gpb_pen.i.y = p->i.y);
	USEUP(2 * sizeof(int) + sizeof(short));
	gpb->gpb_ptr = ptr.sh;			/* update pointer */
}

/****
 *
 * drel_poly_i3d(gpbobj, p)
 * Draw relative polygon 3D
 *
 ****/
static
drel_poly_i3d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/*  GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	draw_poly_i3d();

/*    --uses--			 */
	POINT	pen;

	pen = *p;
	pen.i.x += Get_VP(gpbobj, VP_PEN)->i.x;
	pen.i.y += Get_VP(gpbobj, VP_PEN)->i.y;
	pen.i.z += Get_VP(gpbobj, VP_PEN)->i.z;
	draw_poly_i3d(gpbobj, &pen);
}

/****
 *
 * draw_poly_i3d(gpbobj, p)
 * Append integer 3D polygon edge to GP buffer.
 * (see draw_poly_i2d for details).
 *
 ****/
static
draw_poly_i3d(gpbobj, p)
/*    --needs--			 */
	GPBUF	gpbobj;		/*  GP buffer object */
	POINT	*p;		/* point to add */
{
local	void	gp_start();	/* start new polygon list */

FAST	GPDATA	*gpb;		/* -> GP buffer */
FAST	short	*ptr;		/* shared memory pointer */
	POINT	temp;		/* temporary pen position */

	gpb = ObjAddr(gpbobj, GPDATA);
	if (gpb->gpb_vstart == 0)		/* no polygon started? */
	  {
	   gp_start(gpb);			/* start new polygon */
	   gpb->gpb_poly = gpb->gpb_pen;	/* save start of polygon */
	   draw_poly_i3d(gpbobj, &(gpb->gpb_pen));
	  }
	if (!ROOMFOR(3 * sizeof(int)))
	   {
	    gp_start(gpb);			/* start new polygon list */
	    temp = gpb->gpb_pen;		/* save pen position */
	    draw_poly_i3d(gpbobj, &(gpb->gpb_poly));
	    draw_poly_i3d(gpbobj, &temp);	/* store common edge */
	   }
	ptr = gpb->gpb_ptr;			/* where to put vector */
	PUT_INT(ptr, gpb->gpb_pen.i.x = p->i.x); /* save new edge */
	PUT_INT(ptr, gpb->gpb_pen.i.y = p->i.y);
	PUT_INT(ptr, gpb->gpb_pen.i.z = p->i.z);
	USEUP(3 * sizeof(int));
	if (gpb->gpb_fill == VP_SHADE_GOURAUD)
	  {
	   FAST int v;

	   v = gpb->gpb_vertex_color;
	   PUT_INT(ptr,
		Vertex(gpb->gpb_cindex[VertexColor(v) & gpb->gpb_pixmask],
		VertexShade(v)));
	   USEUP(sizeof(int));
	  }
	gpb->gpb_ptr = ptr;			/* update pointer */
	++(*(gpb->gpb_vecsize));		/* update vector list size */
}

/****
 *
 * drel_vec_i3d(gpbobj, p)
 * Draw relative vector 3D
 *
 ****/
static
drel_vec_i3d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	draw_vec_i3d();

/*    --uses--			 */
	POINT	pen;

	pen = *p;
	pen.i.x += Get_VP(gpbobj, VP_PEN)->i.x;
	pen.i.y += Get_VP(gpbobj, VP_PEN)->i.y;
	pen.i.z += Get_VP(gpbobj, VP_PEN)->i.z;
	draw_vec_i3d(gpbobj, &pen);
}

/****
 *
 * draw_vec_i3d(gpb, p)
 * Append 3D integer vector to GP buffer.
 * (see draw_vec_i2d for details).
 *
 ****/
static
draw_vec_i3d(gpbobj, p)
/*    --needs--			 */
	GPBUF	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	void	gp_start();	/* start new vector/polygon list */

/*    --uses--			 */
FAST	GPDATA	*gpb;
FAST	PTR	ptr;		/* shared memory pointer */

	gpb = ObjAddr(gpbobj, GPDATA);
	if ((gpb->gpb_vstart == 0) ||		/* no list started? */
	    !ROOMFOR(3 * sizeof(int) + sizeof(short)))
	  {
	   gpb->gpb_vstart = 0;
	   move_vec_i3d(gpbobj, &(gpb->gpb_pen));
	  }
	ptr.sh = gpb->gpb_ptr;			/* where to put vector */
	*(ptr.sh - (3 * sizeof(int)) / sizeof(short) - 1) &= ~GPB_END_FLAG;
	PUT_SHORT(ptr.sh, GPB_END_FLAG | GPB_DRAW_FLAG);
	PUT_INT(ptr.sh, gpb->gpb_pen.i.x = p->i.x);
	PUT_INT(ptr.in, gpb->gpb_pen.i.y = p->i.y);
	PUT_INT(ptr.in, gpb->gpb_pen.i.z = p->i.z);
	USEUP(3 * sizeof(int) + sizeof(short));
	gpb->gpb_ptr = ptr.sh;			/* update pointer */
}

/****
 *
 * move_vec_i3d(gpb, p)
 * Append 3D integer vector to GP buffer.
 * (see draw_vec_i2d for details).
 *
 ****/
static
move_vec_i3d(gpbobj, p)
/*    --needs--			 */
	GPBUF	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	void	gp_start();	/* start new vector/polygon list */

/*    --uses--			 */
FAST	GPDATA	*gpb;
FAST	PTR	ptr;		/* shared memory pointer */

	gpb = ObjAddr(gpbobj, GPDATA);
	if ((gpb->gpb_vstart == 0) ||		/* no list started? */
	    !ROOMFOR(3 * sizeof(int) + sizeof(short)))
	  {
	   gp_start(gpb);			/* start vector list */
	   ptr.sh = gpb->gpb_ptr;		/* where to put vector */
	  }
	else
	  {
	   ptr.sh = gpb->gpb_ptr;		/* where to put vector */
	   *(ptr.sh - (3 * sizeof(int)) / sizeof(short) - 1) &= ~GPB_END_FLAG;
	  }
	PUT_SHORT(ptr.sh, GPB_END_FLAG | GPB_MOVE_FLAG);
	PUT_INT(ptr.in, gpb->gpb_pen.i.x = p->i.x);
	PUT_INT(ptr.in, gpb->gpb_pen.i.y = p->i.y);
	PUT_INT(ptr.in, gpb->gpb_pen.i.z = p->i.z);
	USEUP(3 * sizeof(int) + sizeof(short));
	gpb->gpb_ptr = ptr.sh;			/* update pointer */
}

/****
 *
 * drel_poly_f2d(gpbobj, p)
 * Draw relative polygon 2D
 *
 ****/
static
drel_poly_f2d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	draw_poly_f2d();

/*    --uses--			 */
	POINT	pen;

	pen = *p;
	pen.f.x += Get_VP(gpbobj, VP_PEN)->f.x;
	pen.f.y += Get_VP(gpbobj, VP_PEN)->f.y;
	draw_poly_f2d(gpbobj, &pen);
}

/****
 *
 * draw_poly_f2d(gpbobj, p)
 * Append floating 2D polygon edge to GP buffer.
 * (see draw_poly_i2d for details).
 *
 ****/
static
draw_poly_f2d(gpbobj, p)
/*    --needs--			 */
	GPBUF	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
local	void	gp_start();	/* start new polygon list */

FAST	GPDATA	*gpb;		/* -> GP buffer */
FAST	short	*ptr;		/* shared memory pointer */
	POINT	temp;		/* temporary pen position */

	gpb = ObjAddr(gpbobj, GPDATA);
	if (gpb->gpb_vstart == 0)		/* no polygon started? */
	  {
	   gp_start(gpb);			/* start new polygon */
	   gpb->gpb_poly = gpb->gpb_pen;	/* save start of polygon */
	   draw_poly_f2d(gpbobj, &(gpb->gpb_pen)); /* initialize 1st edge */
	  }
	if (!ROOMFOR(2 *sizeof(float)))
	  {
	   gp_start(gpb);			/* start new polygon list */
	   temp = gpb->gpb_pen;			/* save pen position */
	   draw_poly_f2d(gpbobj, &(gpb->gpb_poly));
	   draw_poly_f2d(gpbobj, &temp);	/* store common edge */
	  }
	ptr = gpb->gpb_ptr;			/* where to put vector */
	PUT_FLOAT(ptr, gpb->gpb_pen.f.x = p->f.x);
	PUT_FLOAT(ptr, gpb->gpb_pen.f.y = p->f.y);
	USEUP(2 * sizeof(float));		/* update buffer size */
	++(*(gpb->gpb_vecsize));		/* update vector list size */
	gpb->gpb_ptr = ptr;			/* update pointer */
}

/****
 *
 * drel_vec_f2d(gpbobj, p)
 * Draw relative vector 2D
 *
 ****/
static
drel_vec_f2d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	draw_vec_f2d();

/*    --uses--			 */
	POINT	pen;

	pen = *p;
	pen.f.x += Get_VP(gpbobj, VP_PEN)->f.x;
	pen.f.y += Get_VP(gpbobj, VP_PEN)->f.y;
	draw_vec_f2d(gpbobj, &pen);
}

/****
 *
 * draw_vec_f2d(gpbobj, p)
 * Append 2D floating vector to GP buffer.
 * (see draw_vec_i2d for details).
 *
 ****/
static
draw_vec_f2d(gpbobj, p)
/*    --needs--			 */
	GPBUF	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	void	gp_start();	/* start new vector/polygon list */

/*    --uses--			 */
FAST	GPDATA	*gpb;
FAST	PTR	ptr;		/* shared memory pointer */

	gpb = ObjAddr(gpbobj, GPDATA);
	if ((gpb->gpb_vstart == 0) ||		/* no list started? */
	    !ROOMFOR(2 * sizeof(int) + sizeof(short)))
	  {
	   gpb->gpb_vstart = 0;
	   move_vec_f2d(gpbobj, &(gpb->gpb_pen));
	  }
	ptr.sh = gpb->gpb_ptr;			/* where to put vector */
	*(ptr.sh - (2 * sizeof(float)) / sizeof(short) - 1) &= ~GPB_END_FLAG;
	PUT_SHORT(ptr.sh, GPB_END_FLAG | GPB_DRAW_FLAG);
	PUT_INT(ptr.in, gpb->gpb_pen.i.x = p->i.x);
	PUT_INT(ptr.in, gpb->gpb_pen.i.y = p->i.y);
	USEUP(2 * sizeof(int) + sizeof(short));
	gpb->gpb_ptr = ptr.sh;			/* update pointer */
}

/****
 *
 * move_vec_f2d(gpbobj, p)
 * Append 2D floating vector to GP buffer.
 * (see draw_vec_i2d for details).
 *
 ****/
static
move_vec_f2d(gpbobj, p)
/*    --needs--			 */
	GPBUF	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	void	gp_start();	/* start new vector/polygon list */

/*    --uses--			 */
FAST	GPDATA	*gpb;
FAST	PTR	ptr;		/* shared memory pointer */

	gpb = ObjAddr(gpbobj, GPDATA);
	if ((gpb->gpb_vstart == 0) ||		/* no list started? */
	    !ROOMFOR(2 * sizeof(int) + sizeof(short)))
	  {
	   gp_start(gpb);			/* start vector list */
	   ptr.sh = gpb->gpb_ptr;		/* where to put vector */
	  }
	else
	  {
	   ptr.sh = gpb->gpb_ptr;		/* where to put vector */
	   *(ptr.sh - (2 * sizeof(float)) / sizeof(short) - 1) &= ~GPB_END_FLAG;
	  }
	PUT_SHORT(ptr.sh, GPB_END_FLAG | GPB_MOVE_FLAG);
	PUT_INT(ptr.in, gpb->gpb_pen.i.x = p->i.x);
	PUT_INT(ptr.in, gpb->gpb_pen.i.y = p->i.y);
	USEUP(2 * sizeof(int) + sizeof(short));
	gpb->gpb_ptr = ptr.sh;			/* update pointer */
}

/****
 *
 * drel_poly_f3d(gpbobj, p)
 * Draw relative polygon 3D
 *
 ****/
static
drel_poly_f3d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	draw_poly_f3d();

/*    --uses--			 */
	POINT	pen;

	pen = *p;
	pen.f.x += Get_VP(gpbobj, VP_PEN)->f.x;
	pen.f.y += Get_VP(gpbobj, VP_PEN)->f.y;
	pen.f.z += Get_VP(gpbobj, VP_PEN)->f.z;
	draw_poly_f3d(gpbobj, &pen);
}

/****
 *
 * draw_poly_f3d(gpbobj, p)
 * Append floating 3D polygon edge to GP buffer.
 * (see draw_poly_i2d for details).
 *
 ****/
static
draw_poly_f3d(gpbobj, p)
/*    --needs--			 */
	GPBUF	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
local	void	gp_start();	/* start new polygon list */

FAST	GPDATA	*gpb;
FAST	short	*ptr;		/* shared memory pointer */
	POINT	temp;		/* temporary pen position */

	gpb = ObjAddr(gpbobj, GPDATA);
	if (gpb->gpb_vstart == 0) 		/* no polygon started? */
	  {
	   gp_start(gpb);			/* start new polygon */
	   gpb->gpb_poly = gpb->gpb_pen;	/* save start of polygon */
	   draw_poly_f3d(gpbobj, &(gpb->gpb_pen)); /* initialize 1st edge */
	  }
	if (!ROOMFOR((3 * sizeof(float)) + sizeof(int)))
	   {
	    gp_start(gpb);			/* start new polygon list */
	    temp = gpb->gpb_pen;		/* save pen position */
	    draw_poly_f3d(gpbobj, &(gpb->gpb_poly));
	    draw_poly_f3d(gpbobj, &temp);	/* store common edge */
	   }
	ptr = gpb->gpb_ptr;			/* where to put vector */
	PUT_FLOAT(ptr, gpb->gpb_pen.f.x = p->f.x);
	PUT_FLOAT(ptr, gpb->gpb_pen.f.y = p->f.y);
	PUT_FLOAT(ptr, gpb->gpb_pen.f.z = p->f.z);
	USEUP(3 * sizeof(float));		/* update buffer size */
	if (gpb->gpb_fill == VP_SHADE_GOURAUD)
	  {
	   FAST int v;

	   v = gpb->gpb_vertex_color;
	   PUT_INT(ptr,
		Vertex(gpb->gpb_cindex[VertexColor(v) & gpb->gpb_pixmask],
		VertexShade(v)));
	   USEUP(sizeof(int));
	  }
	++(*(gpb->gpb_vecsize));		/* update vector list size */
	gpb->gpb_ptr = ptr;			/* update pointer */
}

/****
 *
 * drel_vec_f3d(gpbobj, p)
 * Draw relative vector 3D
 *
 ****/
static
drel_vec_f3d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	draw_vec_f3d();

/*    --uses--			 */
	POINT	pen;

	pen = *p;
	pen.f.x += Get_VP(gpbobj, VP_PEN)->f.x;
	pen.f.y += Get_VP(gpbobj, VP_PEN)->f.y;
	pen.f.z += Get_VP(gpbobj, VP_PEN)->f.z;
	draw_vec_f3d(gpbobj, &pen);
}

/****
 *
 * draw_vec_f3d(gpbobj, p)
 * Append 3D floating vector to GP buffer.
 * (see draw_vec_i2d for details).
 *
 ****/
static
draw_vec_f3d(gpbobj, p)
/*    --needs--			 */
	GPBUF	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	void	gp_start();	/* start new vector/polygon list */

/*    --uses--			 */
FAST	GPDATA	*gpb;
FAST	PTR	ptr;		/* shared memory pointer */

	gpb = ObjAddr(gpbobj, GPDATA);
	if ((gpb->gpb_vstart == 0) ||		/* no list started? */
	    !ROOMFOR(3 * sizeof(float) + sizeof(short)))
	  {
	   gpb->gpb_vstart = 0;
	   move_vec_f3d(gpbobj, &(gpb->gpb_pen));
	  }
	ptr.sh = gpb->gpb_ptr;			/* where to put vector */
	*(ptr.sh - (3 * sizeof(float)) / sizeof(short) - 1) &= ~GPB_END_FLAG;
	PUT_SHORT(ptr.sh, GPB_END_FLAG | GPB_DRAW_FLAG);
	PUT_FLOAT(ptr.fl, gpb->gpb_pen.f.x = p->f.x);
	PUT_FLOAT(ptr.fl, gpb->gpb_pen.f.y = p->f.y);
	PUT_FLOAT(ptr.fl, gpb->gpb_pen.f.z = p->f.z);
	USEUP(3 * sizeof(float) + sizeof(short));
	gpb->gpb_ptr = ptr.sh;			/* update pointer */
}

/****
 *
 * move_vec_f3d(gpbobj, p)
 * Append 3D floating vector to GP buffer.
 * (see draw_vec_i2d for details).
 *
 ****/
static
move_vec_f3d(gpbobj, p)
/*    --needs--			 */
	GPBUF	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	void	gp_start();	/* start new vector/polygon list */

/*    --uses--			 */
FAST	GPDATA	*gpb;
FAST	PTR	ptr;		/* shared memory pointer */

	gpb = ObjAddr(gpbobj, GPDATA);
	if ((gpb->gpb_vstart == 0) ||		/* no list started? */
	    !ROOMFOR(3 * sizeof(float) + sizeof(short)))
	  {
	   gp_start(gpb);			/* start vector list */
	   ptr.sh = gpb->gpb_ptr;		/* where to put vector */
	  }
	else
	  {
	   ptr.sh = gpb->gpb_ptr;		/* where to put vector */
	   *(ptr.sh - (3 * sizeof(float)) / sizeof(short) - 1) &= ~GPB_END_FLAG;
	  }
	PUT_SHORT(ptr.sh, GPB_END_FLAG | GPB_MOVE_FLAG);
	PUT_FLOAT(ptr.fl, gpb->gpb_pen.f.x = p->f.x);
	PUT_FLOAT(ptr.fl, gpb->gpb_pen.f.y = p->f.y);
	PUT_FLOAT(ptr.fl, gpb->gpb_pen.f.z = p->f.z);
	USEUP(3 * sizeof(float) + sizeof(short));
	gpb->gpb_ptr = ptr.sh;			/* update pointer */
}

/****
 *
 * move_vec(gpbobj, p)
 * Set pen position. For vectors, nothing is put into GP buffer.
 *
 ****/
move_vec(gpbobj, p)
/*    --needs--			 */
	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
	*Get_VP(gpbobj, VP_PEN) = *p;
}

/****
 *
 * move_poly_i2d(gpbobj, p)
 * Set integer 2D pen position. For polygons, the move operation
 * initializes a new bounded polygon.
 *
 ****/
move_poly_i2d(gpbobj, p)
/*    --needs--			 */
	GPBUF	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	draw_poly_i2d(); /* append vector */
local	int	gp_poly_bound();	/* establish new polygon bound */
FAST	GPDATA	*gpb;		/* -> GP buffer */

	gpb = ObjAddr(gpbobj, GPDATA);
	gpb->gpb_pen.i.x = p->i.x;		/* stuff pen position */
	gpb->gpb_pen.i.y = p->i.y;
	if (gpb->gpb_vstart &&			/* polygon open? */
	    (*(gpb->gpb_vecsize) > 1))		/* complete edge? */
	   if (gp_poly_bound(gpb))		/* new bound OK? */
	     {
	      gpb->gpb_poly = gpb->gpb_pen;	/* save starting position */
	      draw_poly_i2d(gpbobj, p);		/* add new edge */
	     }
}

/****
 *
 * mrel_poly_i2d(gpbobj, p)
 * Move relative (Adds to pen position) integer 2D polygon.
 *
 ****/
mrel_poly_i2d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	move_poly_i2d();
	POINT	pen;

	pen = *p;
	pen.i.x += Get_VP(gpbobj, VP_PEN)->i.x;
	pen.i.y += Get_VP(gpbobj, VP_PEN)->i.y;
	move_poly_i2d(gpbobj, &pen);
}

/****
 *
 * mrel_vec_i2d(gpbobj, p)
 * Move relative (Adds to pen position) integer 2D vector.
 *
 ****/
mrel_vec_i2d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
	Get_VP(gpbobj, VP_PEN)->i.x += p->i.x;
	Get_VP(gpbobj, VP_PEN)->i.y += p->i.y;
	move_vec_i2d(gpbobj, Get_VP(gpbobj, VP_PEN));
}

/****
 *
 * mrel_vec_i3d(gpbobj, p)
 * Move relative (Adds to pen position) integer 3D vector.
 *
 ****/
mrel_vec_i3d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
	Get_VP(gpbobj, VP_PEN)->i.x += p->i.x;
	Get_VP(gpbobj, VP_PEN)->i.y += p->i.y;
	Get_VP(gpbobj, VP_PEN)->i.z += p->i.z;
	move_vec_i3d(gpbobj, Get_VP(gpbobj, VP_PEN));
}

/****
 *
 * move_poly_i3d(gpbobj, p)
 * Set integer 3D pen position. For polygons, the move operation
 * initializes a new bounded polygon.
 *
 ****/
move_poly_i3d(gpbobj, p)
/*    --needs--			 */
FAST	GPBUF	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	draw_poly_i3d(); /* append vector */
local	int	gp_poly_bound();	/* establish new polygon bound */

FAST	GPDATA	*gpb;		/* -> GP buffer */

	gpb = ObjAddr(gpbobj, GPDATA);
	gpb->gpb_pen = *p;
	if (gpb->gpb_vstart &&			/* polygon open? */
	    (*(gpb->gpb_vecsize) > 1))		/* complete edge? */
	   if (gp_poly_bound(gpb))		/* new bound OK? */
	     {
	      gpb->gpb_poly = gpb->gpb_pen;	/* save starting position */
	      draw_poly_i3d(gpbobj, p);		/* add new edge */
	     }
}

/****
 *
 * mrel_poly_i3d(gpbobj, p)
 * Move relative (Adds to pen position) integer 3D polygon.
 *
 ****/
mrel_poly_i3d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	move_poly_i3d();
	POINT	pen;

	pen = *p;
	pen.i.x += Get_VP(gpbobj, VP_PEN)->i.x;
	pen.i.y += Get_VP(gpbobj, VP_PEN)->i.y;
	pen.i.z += Get_VP(gpbobj, VP_PEN)->i.z;
	move_poly_i3d(gpbobj, &pen);
}

/****
 *
 * move_poly_f3d(gpb, p)
 * Set floating 3D pen position. For polygons, the move operation
 * initializes a new bounded polygon.
 *
 ****/
move_poly_f3d(gpbobj, p)
/*    --needs--			 */
FAST	GPBUF	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	draw_poly_f3d(); /* append vector */
local	int	gp_poly_bound();/* establish new polygon bound */

FAST	GPDATA	*gpb;		/* -> GP buffer */

	gpb = ObjAddr(gpbobj, GPDATA);
	gpb->gpb_pen = *p;
	if (gpb->gpb_vstart &&			/* polygon open? */
	    (*(gpb->gpb_vecsize) > 1))		/* complete edge? */
	   if (gp_poly_bound(gpb))			/* new bound OK? */
	     {
	      gpb->gpb_poly = gpb->gpb_pen;	/* save starting position */
	      draw_poly_f3d(gpbobj, p);		/* add new edge */
	     }
}

/****
 *
 * mrel_poly_f3d(gpbobj, p)
 * Move relative (Adds to pen position) floating 3D polygon.
 *
 ****/
mrel_poly_f3d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	move_poly_f3d();
	POINT	pen;

	pen = *p;
	pen.f.x += Get_VP(gpbobj, VP_PEN)->f.x;
	pen.f.y += Get_VP(gpbobj, VP_PEN)->f.y;
	pen.f.z += Get_VP(gpbobj, VP_PEN)->f.z;
	move_poly_f3d(gpbobj, &pen);
}

/****
 *
 * mrel_vec_f2d(gpbobj, p)
 * Move relative (Adds to pen position) floating 2D vector.
 *
 ****/
mrel_vec_f2d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
	Get_VP(gpbobj, VP_PEN)->f.x += p->f.x;
	Get_VP(gpbobj, VP_PEN)->f.y += p->f.y;
	move_vec_f2d(gpbobj, Get_VP(gpbobj, VP_PEN));
}

/****
 *
 * mrel_vec_f3d(gpbobj, p)
 * Move relative (Adds to pen position) floating 3D vector.
 *
 ****/
mrel_vec_f3d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
	Get_VP(gpbobj, VP_PEN)->f.x += p->f.x;
	Get_VP(gpbobj, VP_PEN)->f.y += p->f.y;
	Get_VP(gpbobj, VP_PEN)->f.z += p->f.z;
	move_vec_f3d(gpbobj, Get_VP(gpbobj, VP_PEN));
}

/****
 *
 * move_poly_f2d(gpbobj, p)
 * Set floating 2D pen position. For polygons, the move operation
 * initializes a new bounded polygon.
 *
 ****/
move_poly_f2d(gpbobj, p)
/*    --needs--			 */
FAST	GPBUF	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	draw_poly_f2d(); /* append vector */
local	int	gp_poly_bound();	/* establish new polygon bound */
FAST	GPDATA	*gpb;

	gpb = ObjAddr(gpbobj, GPDATA);
	gpb->gpb_pen = *p;
	if (gpb->gpb_vstart &&			/* polygon open? */
	    (*(gpb->gpb_vecsize) > 1))		/* complete edge? */
	   if (gp_poly_bound(gpb))		/* new bound OK? */
	     {
	      gpb->gpb_poly = gpb->gpb_pen;	/* save starting position */
	      draw_poly_f2d(gpbobj, p);		/* add new edge */
	     }
}

/****
 *
 * mrel_poly_f2d(gpbobj, p)
 * Move relative (Adds to pen position) floating 2D polygon.
 *
 ****/
mrel_poly_f2d(gpbobj, p)
/*    --needs--			 */
FAST	VP	gpbobj;		/* GP buffer object */
	POINT	*p;		/* point to add */
{
/*    --calls--			 */
local	int	move_poly_f2d();
	POINT	pen;

	pen = *p;
	pen.f.x += Get_VP(gpbobj, VP_PEN)->f.x;
	pen.f.y += Get_VP(gpbobj, VP_PEN)->f.y;
	move_poly_f2d(gpbobj, &pen);
}

/****
 *
 * box_gp(gpbobj, dim)
 * Draw a 2D box of dimensions DIM.X by DIM.Y. The color and mode are taken
 * from the last GPB_COLOR and GPB_ROP values. The upper left corner of
 * the box is at the pen position. The first two coordinates of POINT
 * dim are taken as DX and DY (the Z is ignored). Depending on VP_FILL,
 * the box will be drawn as a vector outline or a filled polygon.
 *
 * notes:
 * box_gp does not know which coordinate type it is working with.
 * That is why the code is so confusing - it has to work with
 * integer/floating 2D/3D without doing arithmetic operations on
 * the arguments.
 *
 ****/
box_gp(gpbobj, dim)
/*    --needs--			 */
FAST	GPBUF	gpbobj;		/* GP buffer object */
	XYZ	*dim;		/* dimensions */
{
	XYZ	p, q;

	q = *((XYZ *) Get_VP(gpbobj, VP_PEN));	/* save original X, Y */
	p.y.i = p.z.i = 0; p.x = dim->x;
	DrawRel_VP(gpbobj, &p);			/* draw to upper right corner */
	p.x.i = 0; p.y = dim->y;		/* bottom Y */
	DrawRel_VP(gpbobj, &p);			/* draw to lower right corner */
	p.x = q.x;
	p.y = ((XYZ *) Get_VP(gpbobj, VP_PEN))->y;
	p.z = ((XYZ *) Get_VP(gpbobj, VP_PEN))->z;
	Draw_VP(gpbobj, &p);			/* draw to lower left corner */
	Draw_VP(gpbobj, &q);			/* draw to upper left corner */
}

/****
 *
 * oval_gp(gpbobj, r)
 * Draw a 2D oval of dimensions DIM.X by DIM.Y. The color and mode are taken
 * from the last GPB_COLOR and GPB_ROP values. The center of the oval
 * is at the pen position. The first two coordinates of R are taken
 * as DX and DY (the Z is ignored). Depending on VP_FILL,
 * the oval will be drawn as a vector outline or a filled polygon.
 *
 * notes:
 * The oval code is a mess because the GP does not render anything but
 * vectors and polygons so the oval has to be expressed in terms of these.
 * We want to render in physical coordinates so that we will do the
 * correct number of vectors or edges. We use the GP for rendering (vs
 * just accessing the screen directly) so that ovals can be transformed
 * (scaled, rotated, etc.) Switching back and forth between user and
 * physical coordinates is messy.
 * 
 ****/
oval_gp(gpbobj, r)
/*    --needs--			 */
FAST	VP	gpbobj;		/* viewport */
FAST	POINT	*r;		/* radii */
{
/*    --calls--			 */
local	void	vp_frameoval();	/* draw outlined oval with vectors */

/*    --uses--			 */
	POINT	org, dim;
	POINT	view_org, view_dim;	/* viewport origin & dimensions */
	POINT	zero, oldtrans, newtrans;
	MATRIX	mat;		/* user matrix */
	int	matrix, ctype;	/* coordinate types */

/*
 * Establish new viewing parameters. We need to get the pen position
 * and ellipse dimensions in PIXEL COORDINATES. To do this, we load
 * an identity matrix (to remove any transformations) and get the
 * physical coordinates of the center and dimensions. The user's matrix
 * (if any) is re-established (so the ellipse will be transformed), but
 * the ellipse is plotted in physical coordinates (NULL VIEW_ORG/VIEW_DIM)
 */
	view_org = *((POINT *) Get_VP(gpbobj, VP_VIEW_ORG));
	view_dim = *((POINT *) Get_VP(gpbobj, VP_VIEW_DIM));
	org = *((POINT *) Get_VP(gpbobj, VP_PEN));	/* center of oval */
	MoveRel_VP(gpbobj, r);			  	/* point on perimeter */
	dim = *((POINT *) Get_VP(gpbobj, VP_PEN));	/* center of oval */
	if (matrix = Get_VP(gpbobj, VP_MATRIX))
	  {
	   Get_Matrix(gpbobj, matrix, mat);		/* read trans factors */
	   Set_VP(gpbobj, VP_MATRIX, 0);		/* use identity */
	  }
	Xform_VP(gpbobj, &org, &dim);			/* get pixel coords */
	dim.i.x -= org.i.x;				/* major & minor axes */
	dim.i.y -= org.i.y;
/*
 * Because the translation components of the user matrix will not preserve
 * correctly across a viewport coordinate system change, we save them and
 * subtract them from the pen position (so that, when the user matrix is
 * applied there is no net translation). The translation components from
 * the matrix are converted to physical coordinates and added to the pen
 * so we get the correct translation.
 */
	if (matrix)					/* xform matrix? */
	  {
	   Point_I2D(&zero, 0, 0);			/* origin */
	   Point_I2D(&oldtrans, mat[3][0], mat[3][1]);	/* from user matrix */
	   newtrans = oldtrans;
	   Xform_VP(gpbobj, &newtrans, &zero);		/* get pixel coords */
	   Set_VP(gpbobj, VP_MATRIX, matrix);		/* restore matrix */
	   org.i.x += newtrans.i.x - zero.i.x - oldtrans.i.x;
	   org.i.y += newtrans.i.y - zero.i.y - oldtrans.i.y;
	  }
	ctype = Get_VP(gpbobj, VP_COORD);
	Set_VP(gpbobj, VP_VIEW_DIM, NULL);
	Set_VP(gpbobj, VP_COORD, Type2D(ctype) ? VP_INT2D : VP_INT3D);
/*
 * Display the ellipse. The ellipse is drawn as an outline. If VP_FILL
 * mode is set, the outline will be decomposed into filled polygons.
 */
	LOCK_VP(gpbobj);
	vp_frameoval(gpbobj, &org, dim.i.x, dim.i.y); /* draw oval as outline */
	UNLOCK_VP(gpbobj);
/*
 * reload original viewing parameters
 */
	Move_VP(gpbobj, &org);
	Set_VP(gpbobj, VP_COORD, ctype);
	Set_VP(gpbobj, VP_VIEW_ORG, &view_org);
	Set_VP(gpbobj, VP_VIEW_DIM, &view_dim);
}

/****
 *
 * get_gp(gpbobj, attr)
 * Get GP attribute value.
 *
 * set_gp(gpbobj, attr, val)
 * Set GP attribute.
 *
 * The GP-specific attributes are:
 *
 *   VP_AUTO_SCALE	0 = fixed size image
 *			VP_SCALE_ALL scale automatically to window
 *			VP_SCALE_SQUARE scale to square aspect ratio
 *   VP_SIZE		size of GP command buffer in blocks
 *   VP_CLIPPLANES	clipping planes
 *   VP_HIDDENSURF	hidden surface
 *   VP_MATRIX		matrix selected
 *   VP_WIN_ORG		(POINT) origin of visible window area
 *   VP_WIN_DIM		(POINT) dimensions of visible window area
 *   VP_CLIP_ORG	(POINT) origin of clip area
 *   VP_CLIP_DIM	(POINT) dimensions of clip area
 *   VP_VERTEX_COLOR	color of GOURAUD shaded polygon vertex
 *
 *   VP_ROP		raster op
 *   VP_COLOR		color
 *   VP_FILL		set if polygons filled, clear for outline only
 *   VP_VIEW_ORG	(POINT) origin of view area
 *   VP_VIEW_DIM	(POINT) dimensions of view area
 *   VP_PEN		(POINT) current position
 *   VP_PIXPLANES	pixel mask planes
 *   VP_FRAME		frame number (for double buffering)
 *   VP_PIXBITS		number of bits per pixel
 *   VP_DBUF		double buffering options
 *   VP_CMAP_RED	red color map
 *   VP_CMAP_GREEN	green color map
 *   VP_CMAP_BLUE	blue color map
 *   VP_CMAP_SIZE	color map size
 *   VP_PICK_ID		pick ID
 *   VP_PICK_COUNT	pick count
 *   VP_PICK_ORG	pick area origin
 *   VP_PICK_DIM	pick area dimensions
 *
 *   vp_DBUFPLANES	double buffering planes mask (internal use only)
 *
 ****/
get_gp(gpbobj, attr)
/*    --needs--			 */
	GPBUF	gpbobj;
	int	attr;
{
/*    --calls--			 */
extern	int	vp_pick_get();

/*    --uses--			 */
FAST	GPDATA	*gpb;
static	POINT	p;

	gpb = ObjAddr(gpbobj, GPDATA);
	switch (attr)
       {
	case VP_WIN: return ((int) gpb->gpb_win);
	case VP_TYPE: return (gpb->gpb_type);
	case VP_COORD: return (gpb->gpb_coord);
	case VP_COLOR: return (gpb->gpb_color);
	case VP_ROP: return (gpb->gpb_rop);
	case VP_FILL: return (gpb->gpb_fill);
	case VP_VISIBLE: return (gpb->gpb_visible);
	case VP_PATTERN: return (gpb->gpb_pattern);
	case VP_LINE_STYLE: return (gpb->gpb_line_style);
	case VP_LINE_WIDTH: return (gpb->gpb_line_width);
	case VP_LINE_HEIGHT: return (gpb->gpb_line_height);
	case VP_LINE_SIZE: return (gpb->gpb_line_size);
	case VP_PEN: return ((int) &(gpb->gpb_pen));
	case VP_CMAP_RED: return ((int) ((VPDATA *) gpb)->vp_cmap_red);
	case VP_CMAP_GREEN: return ((int) ((VPDATA *) gpb)->vp_cmap_green);
	case VP_CMAP_BLUE: return ((int) ((VPDATA *) gpb)->vp_cmap_blue);
	case VP_CMAP_SIZE: return (((VPDATA *) gpb)->vp_cmap_size);
	case VP_DBUF_STAT: return (gpb->gpb_dbuf_stat);
	case VP_PIXPLANES: return (gpb->gpb_pixplanes);
	case VP_PIXBITS: return (gpb->gpb_pixbits);
	case VP_FRAME: return (gpb->gpb_frame);
	case VP_VERTEX_COLOR: return (gpb->gpb_vertex_color);
	case VP_PICK_COUNT: return (gpb->gpb_pick_count);
	case VP_PICK_ORG: return ((int) &(gpb->gpb_pick_org));
	case VP_PICK_DIM: return ((int) &(gpb->gpb_pick_dim));
	case VP_VIEW_ORG: return ((int) &(gpb->gpb_VIEW_ORG));
	case VP_VIEW_DIM: return ((int) &(gpb->gpb_VIEW_DIM));
	case VP_WIN_WIDTH: return (((VPDATA *) gpb)->vp_win_width);
	case VP_WIN_HEIGHT: return (((VPDATA *) gpb)->vp_win_height);

	case VP_PICK_ID:
	return (gpb->gpb_pick_id ? vp_pick_get(gpbobj, VP_PICK_ID) : 0);

/*
 * VIS_ORG and VIS_DIM are just another way of specifying WIN_ORG and WIN_DIM.
 * While WIN_ORG and WIN_DIM are normalized and relative to the window,
 * VIS_ORG and VIS_DIM are specified in terms of the current viewport
 * coordinate system. VIS_ORG and VIS_DIM are calculated by assuming the
 * user viewport is mapped to the entire window. VIS_ORG/VIS_DIM describe
 * which part of the window to actually use for display by setting
 * WIN_ORG and WIN_DIM appropriately. We do not keep copies of VIS_ORG
 * and VIS_DIM - they must be recalculated from WIN_ORG and WIN_DIM
 * when the user inquires their values.
 */
	case GPB_VIS_ORG:
	if (gpb->gpb_flags & GPB_VIEW_FLAG) gp_setview(gpb);
	if (TypeFlt(gpb->gpb_coord))
	   return ((int) Point_F3D(&p, 
		gpb->gpb_view_scale.f.x * (2.0 *
			gpb->gpb_WIN_ORG.f.x - 1.0) + gpb->gpb_view_trans.f.x,
		gpb->gpb_view_scale.f.y * (-2.0 *
			gpb->gpb_WIN_ORG.f.y + 1.0) + gpb->gpb_view_trans.f.y,
		gpb->gpb_view_scale.f.z * (2.0 *
			gpb->gpb_WIN_ORG.f.z - 1.0) + gpb->gpb_view_trans.f.z));
	else return ((int) Point_I3D(&p,
		gpb->gpb_view_scale.f.x * (2.0 *
			gpb->gpb_WIN_ORG.f.x - 1.0) + gpb->gpb_view_trans.f.x,
		gpb->gpb_view_scale.f.y * (-2.0 *
			gpb->gpb_WIN_ORG.f.y + 1.0) + gpb->gpb_view_trans.f.y,
		gpb->gpb_view_scale.f.z * (2.0 *
			gpb->gpb_WIN_ORG.f.z - 1.0) + gpb->gpb_view_trans.f.z));

	case GPB_VIS_DIM:
	if (gpb->gpb_flags & GPB_VIEW_FLAG) gp_setview(gpb);
	if (TypeFlt(gpb->gpb_coord))
	   return ((int) Point_F3D(&p,
		gpb->gpb_view_scale.f.x * gpb->gpb_WIN_DIM.f.x * 2.0,
		-gpb->gpb_view_scale.f.y * gpb->gpb_WIN_DIM.f.y * 2.0,
		gpb->gpb_view_scale.f.z * gpb->gpb_WIN_DIM.f.z * 2.0));
	return ((int) Point_I3D(&p,
		gpb->gpb_view_scale.f.x * gpb->gpb_WIN_DIM.f.x * 2.0,
		-gpb->gpb_view_scale.f.y * gpb->gpb_WIN_DIM.f.y * 2.0,
		gpb->gpb_view_scale.f.z * gpb->gpb_WIN_DIM.f.z * 2.0));

	default: return (0);
       }
}

set_gp(gpbobj, attr, val)
/*    --needs--			 */
FAST	GPBUF	gpbobj;
	uint	attr;
	uint	val;
{
/*    --uses--			 */
FAST	GPDATA	*gpb;
local	_vpfp	*vpftab[];	/* -> GP function dispatch tables */
FAST	POINT	*p;

/*    --calls--                  */
extern  int	vp_dbuf_create();
extern  int     vp_dbuf_set();
extern  int	vp_pick_set();
local	void	gp_clearzbuf();

	gpb = ObjAddr(gpbobj, GPDATA);
/*
 * VP_VERTEX_COLOR vertex color
 * Does not close the current polygon, changes the vertex color
 * for GOURAUD shading.
 */
	if (attr == VP_VERTEX_COLOR)
	  {
	   gpb->gpb_vertex_color = val;
	   return;
	  }
	gpb->gpb_vstart = 0;
	switch (attr)
       {
/*
 * VP_PEN	pen position (same as Move)
 */
	case VP_PEN:
	if ((p = (POINT *) val) == NULL) return;
	Move_VP(gpbobj, p);
	break;

/*
 * VP_COORD	coordinate type (VP_INT2D, VP_INT3D, VP_FLT2D, VP_FLT3D)
 * It is possible to switch between integer and floating coordinates,
 * but not between 2D and 3D. (This would involve resetting the viewport,
 * reloading matrices, etc.) When the coordinate type is changed, the
 * function dispatch pointer (vp_func) is set to refer to the new
 * dispatch table for that type.
 */
	case VP_COORD:
	if (Type2D(gpb->gpb_coord) && Type3D(val))
	   error("Set_VP: you are not allowed to change from 2D to 3D\n", 0, 0);
	if (Type3D(gpb->gpb_coord) && Type2D(val))
	   error("Set_VP: you are not allowed to change from 3D to 2D\n", 0, 0);
	if (gpb->gpb_flags & GPB_VIEW_FLAG) gp_setview(gpb);
	if (gpb->gpb_flags & GPB_CLIP_FLAG) gp_setclip(gpb);
	gpb->gpb_coord = (val &= GPB_COORDMSK);
	gpb->gpb_ftype = (gpb->gpb_ftype & ~GPB_COORDMSK) | val;
	gpb->gpb_vp.vp_func = vpftab[gpb->gpb_ftype];
	break;

/*
 * VP_ROP	raster OP
 * causes any buffered vector list or polygon to be closed
 */
	case VP_ROP:
	gpb->gpb_rop = val;
	MAKEROOM(2 * sizeof(short));
	PUT_CMD(gpb->gpb_ptr, GP1_SET_ROP, 0);
	PUT_SHORT(gpb->gpb_ptr, val);
	break;

/*
 * VP_COLOR	color
 * Causes any buffered vector list or polygon to be closed
 *
 * VP_VERTEX_COLOR vertex color
 * Does not close the current polygon, changes the vertex color
 * for GOURAUD shading. This is a kludge - it should really
 * use VP_COLOR. What can I say - we were in a hurry to get this
 * working for Siggraph.
 */
	case VP_COLOR:
	gpb->gpb_color = val;
	gpb->gpb_vertex_color = Vertex(val, 0);
	val = gpb->gpb_cindex[val & gpb->gpb_pixmask];
	attr = GP1_SET_COLOR;
gpcmd:	MAKEROOM(sizeof(short));
	PUT_CMD(gpb->gpb_ptr, attr, (val & 0XFF));
	break;

	case VP_VERTEX_COLOR:
	gpb->gpb_vertex_color = val;
	break;

/*
 * VP_FILL	nonzero = fill move/draw sequences, else outline only
 *   VP_SHADE_SOLID = solid polygons
 *   VP_SHADE_GOURAUD = GOURAUD shaded polygons
 * When the fill value is changed, the coordinate type is different and
 * function dispatch pointer (vp_func) is set to refer to the new
 * dispatch table for that type.
 */
	case VP_FILL:
	if ((gpb->gpb_fill = val) == 0)
	   gpb->gpb_ftype &= ~GPB_POLYMASK;
	else gpb->gpb_ftype |= GPB_POLYMASK;
	gpb->gpb_vp.vp_func = vpftab[gpb->gpb_ftype];
	break;


/*
 * VP_VISIBLE	visibility on / off. If non-zero, drawing is enabled.
 * Otherwise, the pen position is set by primitives, but nothing is drawn.
 */
	case VP_VISIBLE:
	if (gpb->gpb_visible = val)
	   gpb->gpb_ftype &= ~VP_VISMASK;	/* turn drawing on */
	else gpb->gpb_ftype |= VP_VISMASK;	/* make everything invisible */
	gpb->gpb_vp.vp_func = vpftab[gpb->gpb_ftype];
	break;

/*
 * VP_LINE_WIDTH	 set line width in pixels
 * VP_LINE_HEIGHT	 set line height in pixels
 * VP_LINE_SIZE		 set line size (width & height) in pixels
 */
	case VP_LINE_WIDTH:
	case VP_LINE_HEIGHT:
	case VP_LINE_SIZE:
	if ((gpb->gpb_left -= 3 * sizeof(short)) <= 0)
	  {
	   gp_flush(gpb);
	   gpb->gpb_left -= 3 * sizeof(short);
	  }
	gpb->gpb_line_width = gpb->gpb_line_height = gpb->gpb_line_size = val;
	PUT_CMD(gpb->gpb_ptr, GP1_SET_LINE_WIDTH, 0);
	PUT_SHORT(gpb->gpb_ptr, val);
	PUT_SHORT(gpb->gpb_ptr, 0);
	break;

/*
 * VP_LINE_STYLE
 * Indicate which texture to use for drawing lines. Values less than 1
 * will draw solid lines (null texture loaded). Positive values are
 * indexes into the texture table for the viewport (vp_textab). Each
 * texture is an object consisting of a type in the first word followed
 * by the data for the texture. A line style texture can only be type
 * VP_TEX_RANGE. The data for this type is a null-terminated list of
 * shorts indicating the on and off ranges of the texture pattern.
 * If the texture table is not present or the indexed texture is not
 * defined, a solid texture (null texture loaded) will be used. Otherwise,
 * the texture range data is copied from the object into the GP buffer.
 */
	case VP_LINE_STYLE:
       {
	FAST	PTR	ptr;			/* -> GP buffer entry */
	FAST	short	*tex;			/* -> texture range */
		OBJID	texobj;			/* texture range object */
		int	n;			/* size of texture */

	if (val <= 0)				/* not textured? */
	  {
notex:	   MAKEROOM(4 * sizeof(short));
	   ptr.sh = gpb->gpb_ptr;		/* -> buffer */
	   PUT_CMD(ptr.sh, GP1_SET_LINE_TEX, 0);	/* load null texture */
	   PUT_SHORT(ptr.sh, 0); PUT_SHORT(ptr.sh, 0); PUT_SHORT(ptr.sh, 0);
	   gpb->gpb_ptr = ptr.sh;		/* and pointer */
	   break;
	  }
	if (IsNull(gpb->gpb_textab) ||		/* no texture table? */
	    IsNull(texobj =			/* get texture object */
		GetObjVal(gpb->gpb_textab, (val - 1) * sizeof(OBJID))))
	   goto notex;				/* default to solid */
	tex = ObjAddr(texobj, short);		/* -> texture type */
	switch (tex[0])				/* which texture type? */
	  {
	   case VP_TEX_RANGE:
	   n = ObjSize(texobj) - 2 * sizeof(short); /* # bytes in texture */
	   if (n > 16 * sizeof(short))		/* maximum size is 15 entries */
	      n = 16 * sizeof(short);
	   MAKEROOM(n + 4 * sizeof(short));
	   ptr.sh = gpb->gpb_ptr;		/* -> buffer */
	   PUT_CMD(ptr.sh, GP1_SET_LINE_TEX, 0);	/* load null texture */
	   bcopy(tex + 1, ptr.sh, n);
	   ptr.ch += n;
	   PUT_SHORT(ptr.sh, 0); PUT_SHORT(ptr.sh, 0); PUT_SHORT(ptr.sh, 0);
	   break;

	   case VP_TEX_5080:
	   MAKEROOM(16 * sizeof(short));
	   ptr.sh = gpb->gpb_ptr;		/* -> buffer */
	   PUT_CMD(ptr.sh, GP1_SET_EF_TEX, 0);	/* load null texture */
	   bcopy(tex + 1, ptr.sh, 16 * sizeof(short));
	   ptr.sh += 16;
	   break;

	   default:
	   error("Set_VP: line style %d incorrect type for lines\n", val, 0);
	  }
	gpb->gpb_ptr = ptr.sh;
       }
	break;

/*
 * VP_PATTERN
 * Indicate which pattern to use for filling polygons. Values less than 1
 * will draw solid polygons (null texture loaded). Positive values are
 * indexes into the pattern table for the viewport (vp_pattab). Each
 * pattern is a BITMAP object containing the pixel data (in the form of a
 * memory pixrect).
 */
	case VP_PATTERN:			/* polygon pattern */
	if (((gpb->gpb_pattern = val) > 0) &&	/* textured polygon? */
	    !IsNull(gpb->gpb_pattab))		/* have pattern table? */
	gp_load_pat(gpb, GetObjVal(gpb->gpb_pattab, (val - 1) * sizeof(OBJID)));
	else gp_load_pat(gpb, NULL);		/* switch to solid */
	break;

/*
 * VP_PIXBITS number of bits per pixel
 * VP_PIXPLANES	pixel plane mask
 * VP_FRAME frame number
 * VP_DBUF double buffering status
 * VP_DBUFPLANES double buffering planes mask (internal use only)
 */
	case VP_PIXBITS:
	gpb->gpb_pixbits = val;
	if (vp_dbuf_create(gpbobj) == 0) break;
	gpb->gpb_pixmask = val = gpb->gpb_pixplanes;
	attr = GP1_SET_FB_PLANES;
	goto gpcmd;

	case VP_DBUF_STAT:
	if (gpb->gpb_dbuf) vp_dbuf_set(gpbobj, VP_DBUF_STAT, val);
	break;

	case VP_FRAME:
	gpb->gpb_frame = val;
	if (gpb->gpb_dbuf) vp_dbuf_set(gpbobj, VP_FRAME, val);
	break;

	case VP_PIXPLANES:
	gpb->gpb_pixplanes = val;
	if (gpb->gpb_dbuf)
	  {
	   vp_dbuf_set(gpbobj, VP_PIXPLANES, val);
	   break;
	  }

	case VP_REALPLANES:
	gpb->gpb_fbplanes = val;
	attr = GP1_SET_FB_PLANES;
	goto gpcmd;

/*
 * GPB_AUTO_SCALE  if set, scale viewport to window size
 * GPB_SIZE	   # of blocks in GP command buffer
 * GPB_CLIPPLANES	mask for clipping planes
 */
	case GPB_AUTO_SCALE: gpb->gpb_attr.gpa_AUTO_SCALE = val; break;

	case GPB_SIZE: gpb->gpb_size = val; break;

	case GPB_CLIPPLANES:
	gpb->gpb_attr.gpa_CLIPPLANES = val & GPB_CLIP_ALL;
	attr = GP1_SET_CLIP_PLANES;
	goto gpcmd;
	break;

/*
 * GPB_HIDDENSURF	Enable/disable hidden surface elimination
 * 1) If there is no GB, do not enable hidden surface
 * 2) Send SET_HIDDEN_SURF enable/disable to GP
 * 3) If user is enabling, clear Z buffer for entire window
 */
	case GPB_HIDDENSURF:
	if (!gp1_d(gpb->gpb_vp.vp_win->pw_clipdata->pwcd_prmulti)->gbufflag)
	   break;
	MAKEROOM(sizeof(short));		/* enable command */
	gpb->gpb_attr.gpa_HIDDENSURF = val & 1;
	val = val ? GP1_ZBHIDDENSURF : GP1_NOHIDDENSURF;
	PUT_CMD(gpb->gpb_ptr, GP1_SET_HIDDEN_SURF, val);
	if (gpb->gpb_attr.gpa_HIDDENSURF) gp_clearzbuf(gpb);
	break;

/*
 * GPB_MATRIX - transformation matrix to use.
 * Really we always use matrix 0 as the current ransformation matrix.
 * Selecting the user transformation matrix concatenates it with the
 * viewing matrix (matrix 1) and this becomes the "real" transformation.
 */
	case GPB_MATRIX:
	if (gpb->gpb_attr.gpa_MATRIX == val) break;
	gpb->gpb_attr.gpa_MATRIX = val;
	if (val > 1) Mul_Matrix(gpb, val, GPB_VIEWMATRIX, GPB_XFORMMATRIX);
	else
	  {
	   gpb->gpb_attr.gpa_MATRIX = 0;
	   gp_matrix_set(gpb, 0, gpb->gpb_viewmatrix);
	  }
	break;
/*
 * VP_VIEW_ORG	viewport origin.
 * Every time the viewing origin or dimensions are changed,
 * GPB_VIEW_FLAG is set, but nothing is recalculated at this time.
 * This flag is checked whenever something which depend on VIEW_ORG/VIEW_DIM
 * is used (like when a vector or polygon list is started). Currently,
 * the only parameters that depend directly on VIEW_ORG and VIEW_DIM
 * are view_trans and view_scale. There are indirect dependencies
 * (clip_org & clip_dim depend on view_trans and view_scale)
 * which are taken care of below. The general idea here is to defer the
 * calculations as long as possible so the overhead for changing these
 * parameters is small.
 *
 * 1) Ignore if we are not really changing the values
 * 2) If we have pending changes of both clipping and viewing that have
 *    not been applied yet, apply them before changing viewport
 * 3) If the POINT is NULL, default view origin
 * 4) Set GPB_VIEW_FLAG to indicate view_trans/view_scale must be
 *    recalculated
 * 5) Set GPB_XFORM_FLAG to indicate viewing transformation matrix
 *    (which depends on view_trans/view_scale) must be recalculated
 */
	case VP_VIEW_ORG:
	if ((p = (POINT *) val) &&
	    (gpb->gpb_VIEW_ORG.i.x == p->i.x) &&
	    (gpb->gpb_VIEW_ORG.i.y == p->i.y))
	   break;
	if ((gpb->gpb_flags & (GPB_CLIP_FLAG | GPB_VIEW_FLAG)) ==
		(GPB_CLIP_FLAG | GPB_VIEW_FLAG))
	  {
	   gp_setview(gpb);
	   gp_setclip(gpb);
	  }
	if (p) gpb->gpb_VIEW_ORG = *p;
	else if (TypeFlt(gpb->gpb_coord))
	   Point_F3D(&(gpb->gpb_VIEW_ORG), 0, 0, 0);
	else Point_I3D(&(gpb->gpb_VIEW_ORG), 0, 0, 0);
	gpb->gpb_flags |= GPB_VIEW_FLAG | GPB_XFORM_FLAG;
	break;

/*
 * VP_VIEW_DIM	viewport dimensions.
 * 1) Ignore if we are not really changing the values
 * 2) If we have pending changes of both clipping and viewing that have
 *    not been applied yet, apply them before changing viewport
 * 3) If the POINT is NULL, default view origin and dimensions
 * 4) Set GPB_VIEW_FLAG to indicate view_trans/view_scale must be
 *    recalculated
 * 5) Set GPB_XFORM_FLAG to indicate viewing transformation matrix
 *    (which depends on view_trans/view_scale) must be recalculated
 */
	case VP_VIEW_DIM:
	if ((p = (POINT *) val) &&
	    (gpb->gpb_VIEW_DIM.i.x == p->i.x) &&
	    (gpb->gpb_VIEW_DIM.i.y == p->i.y))
	   break;
	if ((gpb->gpb_flags & (GPB_CLIP_FLAG | GPB_VIEW_FLAG)) ==
		(GPB_CLIP_FLAG | GPB_VIEW_FLAG))
	  {
	   gp_setview(gpb);
	   gp_setclip(gpb);
	  }
	if (p) gpb->gpb_VIEW_DIM = *p;
	else if (TypeFlt(gpb->gpb_coord))
	  {
	   Point_F3D(&(gpb->gpb_VIEW_ORG),
		gpb->gpb_wold.f.x * gpb->gpb_WIN_ORG.f.x,
		gpb->gpb_wold.f.y * gpb->gpb_WIN_ORG.f.y,
		0);
	   Point_F3D(&(gpb->gpb_VIEW_DIM),
		gpb->gpb_wold.f.x * gpb->gpb_WIN_DIM.f.x,
		gpb->gpb_wold.f.y * gpb->gpb_WIN_DIM.f.y,
		1);
	  }
	else
	  {
	   Point_I3D(&(gpb->gpb_VIEW_ORG),
		gpb->gpb_wold.f.x * gpb->gpb_WIN_ORG.f.x,
		gpb->gpb_wold.f.y * gpb->gpb_WIN_ORG.f.y,
		0);
	   Point_I3D(&(gpb->gpb_VIEW_DIM),
		gpb->gpb_wold.f.x * gpb->gpb_WIN_DIM.f.x,
		gpb->gpb_wold.f.y * gpb->gpb_WIN_DIM.f.y,
		1);
	  }
	gpb->gpb_flags |= GPB_VIEW_FLAG | GPB_XFORM_FLAG;
	break;

/*
 * VP_PICK_ID	 pick ID. returns previous pick ID if there was a pick.
 * VP_PICK_COUNT pick count
 * VP_PICK_ORG	 pick area origin.
 * VP_PICK_DIM	 pick area dimensions.
 * Pick attributes are controlled by vp_pick_set. All POINT attributes
 * must be passed as integers.
 */
	case VP_PICK_COUNT:
	gpb->gpb_pick_count = val;
	break;

	case VP_PICK_ID:
	return (vp_pick_set(gpbobj, VP_PICK_ID, val));
	break;

	case VP_PICK_ORG:
	case VP_PICK_DIM:
	if (val) vp_pick_set(gpbobj, attr, val);
	break;


/*
 * GPB_CLIP_ORG	viewport clip area origin.
 * Every time the clipping origin or dimensions are changed,
 * GPB_CLIP_FLAG is set, but nothing is recalculated at this time.
 * This flag is checked whenever something which depend on CLIP_ORG/CLIP_DIM
 * is used (like when a vector or polygon list is started). Currently,
 * the only parameters that depend directly on CLIP_ORG and CLIP_DIM
 * are clip_org and clip_dim. There are indirect dependencies
 * (clip_org & clip_dim depend on view_trans and view_scale)
 * The idea is to defer the calculations as long as possible so
 * the overhead for changing these parameters is small.
 *
 * 1) If the POINT is NULL, default clip origin
 * 2) Set GPB_CLIP_FLAG to indicate clip_org/clip_dim must be
 *    recalculated
 * 3) Set GPB_WIN_FLAG to indicate windowing transformation
 *    (which depends on clip_org/clip_dim) must be recalculated
 */
	case GPB_CLIP_ORG:
	if (p = (POINT *) val) gpb->gpb_CLIP_ORG = *p;
	else gpb->gpb_CLIP_ORG = gpb->gpb_VIEW_ORG;
	gpb->gpb_flags |= GPB_CLIP_FLAG | GPB_WIN_FLAG;
	break;

/*
 * GPB_CLIP_DIM	viewport clip area dimensions.
 * 1) If the POINT is NULL, default clip origin and dimensions
 *    Clip area defaults to same size as viewport
 * 2) Set GPB_CLIP_FLAG to indicate clip_org/clip_dim must be
 *    recalculated
 * 3) Set GPB_WIN_FLAG to indicate windowing transformation
 *    (which depends on clip_org/clip_dim) must be recalculated
 */
	case GPB_CLIP_DIM:
	if (p = (POINT *) val) gpb->gpb_CLIP_DIM = *p;
	else
	  {
	   gpb->gpb_CLIP_ORG = gpb->gpb_VIEW_ORG;
	   gpb->gpb_CLIP_DIM = gpb->gpb_VIEW_DIM;
	  }
	gpb->gpb_flags |= GPB_CLIP_FLAG | GPB_WIN_FLAG;
	break;

/*
 * GPB_WIN_ORG origin of upper left corner of visible window area
 * Every time the windowing origin or dimensions are changed,
 * GPB_WIN_FLAG is set, but nothing is recalculated at this time.
 * This flag is checked whenever something which depend on WIN_ORG/WIN_DIM
 * is used. Currently, the only thing that depends directly on WIN_ORG/WIN_DIM
 * is the transformation from GP space to the physical screen window
 * (GP SET_VWP parameters).
 *
 * WIN_ORG and WIN_DIM are floating point parameters that specify the
 * upper left corner and dimensions of the window in a normalized form.
 * To select a rectangle which covers the entire window, WIN_ORG = (0,0,0)
 * and WIN_DIM = (1,1,1). WIN_ORG and WIN_DIM do not need to be reset if
 * any of the other parameters (viewport coordinate space) change or if
 * the window size changes because it is maintained in a relative form.
 *
 * 1) If the POINT is NULL, default windowing origin to upper left
 * 2) Set GPB_WIN_FLAG to recalculate windowing transformation
 */
	case GPB_WIN_ORG:
	if (p = (POINT *) val) gpb->gpb_WIN_ORG = *p;
	else Point_F3D(&(gpb->gpb_WIN_ORG), 0.0, 0.0, 0.0);
	gpb->gpb_flags |= GPB_WIN_FLAG;
	break;

/*
 * GPB_WIN_DIM dimensions of visible window area
 * 1) If the POINT is NULL, default windowing origin to upper left
 *    and dimensions to entire window
 * 2) Set GPB_WIN_FLAG to recalculate windowing transformation
 */
	case GPB_WIN_DIM:
	if (p = (POINT *) val) gpb->gpb_WIN_DIM = *p;
	else
	  {
	   Point_F3D(&(gpb->gpb_WIN_ORG), 0.0, 0.0, 0.0);
	   Point_F3D(&(gpb->gpb_WIN_DIM), 1.0, 1.0, 1.0);
	  }
	gpb->gpb_flags |= GPB_WIN_FLAG;
	break;

/*
 * GPB_VIS_ORG	visible area origin.
 * VIS_ORG and VIS_DIM are just another way of specifying WIN_ORG and WIN_DIM.
 * While WIN_ORG and WIN_DIM are normalized and relative to the window,
 * VIS_ORG and VIS_DIM are specified in terms of the current viewport
 * coordinate system. VIS_ORG and VIS_DIM are calculated by assuming the
 * user viewport is mapped to the entire window. VIS_ORG/VIS_DIM describe
 * which part of the window to actually use for display by setting
 * WIN_ORG and WIN_DIM appropriately. Because these calculations involve the
 * viewport to GP space factors (view_trans / view_scale), we must recompute
 * these if they have changed.
 *
 * 1) If POINT is NULL, default VIS_ORG to upper left
 * 2) If viewing parameters have changed, recompute view_trans/view_scale
 * 3) Compute WIN_ORG/WIN_DIM
 * 4) Set GPB_WIN_FLAG to recalculate windowing transformation
 */
	case GPB_VIS_ORG:
	if (p = (POINT *) val)		/* user specified VIS_ORG? */
	  {
	   if (gpb->gpb_flags & GPB_VIEW_FLAG) gp_setview(gpb);
	   switch (gpb->gpb_coord)
	  {
	   case VP_FLT3D:
	   gpb->gpb_WIN_ORG.f.z = ((p->f.z - gpb->gpb_view_trans.f.z) /
		gpb->gpb_view_scale.f.z + 1.0) / 2.0;
	   goto voflt;

	   case VP_FLT2D:
	   gpb->gpb_WIN_ORG.f.z = 0.0;
voflt:	   Point_F2D(&(gpb->gpb_WIN_ORG),
		((p->f.x - gpb->gpb_view_trans.f.x) /
			gpb->gpb_view_scale.f.x + 1.0) / 2.0,
		-((p->f.y - gpb->gpb_view_trans.f.y) /
			gpb->gpb_view_scale.f.y - 1.0) / 2.0);
	   break;

	   case VP_INT3D:
	   gpb->gpb_WIN_ORG.f.z = ((p->i.z - gpb->gpb_view_trans.f.z) /
		gpb->gpb_view_scale.f.z + 1.0) / 2.0;
	   goto voint;

	   case VP_INT2D:
	   gpb->gpb_WIN_ORG.f.z = 0.0;
voint:	   Point_I2D(&(gpb->gpb_WIN_ORG),
		((p->i.x - gpb->gpb_view_trans.f.x) /
			gpb->gpb_view_scale.f.x + 1.0) / 2.0,
		-((p->i.y - gpb->gpb_view_trans.f.y) /
			gpb->gpb_view_scale.f.y - 1.0) / 2.0);
	   break;
	  }
	  }
	else Point_F3D(&(gpb->gpb_WIN_ORG), 0.0, 0.0, 0.0);
	gpb->gpb_flags |= GPB_WIN_FLAG;
	break;

/*
 * GPB_VIS_DIM	visible area dimensions in viewport coordinates.
 *
 * 1) If POINT is NULL, default VIS_ORG to upper left, VIS_DIM to whole window
 * 2) If viewing parameters have changed, recompute view_trans/view_scale
 * 3) Compute WIN_ORG/WIN_DIM
 * 4) Set GPB_WIN_FLAG to recalculate windowing transformation
 */
	case GPB_VIS_DIM:
	if (p = (POINT *) val)		/* user specified VIS_DIM? */
	  {
	   if (gpb->gpb_flags & GPB_VIEW_FLAG) gp_setview(gpb);
	   switch (gpb->gpb_coord)
	  {
	   case VP_FLT3D:
	   Point_F3D(&(gpb->gpb_WIN_DIM),
		p->f.x / (gpb->gpb_view_scale.f.x * 2.0),
		-p->f.y / (gpb->gpb_view_scale.f.y * 2.0),
		p->f.z / (gpb->gpb_view_scale.f.z * 2.0));
	   break;

	   case VP_FLT2D:
	   Point_F2D(&(gpb->gpb_WIN_DIM),
		p->f.x / (gpb->gpb_view_scale.f.x * 2.0),
		-p->f.y / (gpb->gpb_view_scale.f.y * 2.0));
	   break;

	   case VP_INT3D:
	   Point_I3D(&(gpb->gpb_WIN_DIM),
		p->f.x / (gpb->gpb_view_scale.f.x * 2.0),
		-p->f.y / (gpb->gpb_view_scale.f.y * 2.0),
		p->f.z / (gpb->gpb_view_scale.f.z * 2.0));
	   break;

	   case VP_INT2D:
	   Point_I2D(&(gpb->gpb_WIN_DIM),
		p->f.x / (gpb->gpb_view_scale.f.x * 2.0),
		-p->f.y / (gpb->gpb_view_scale.f.y * 2.0));
	   break;
	  }
	  }
	else				/* VIS_DIM defaulted */
	  {
	   Point_F3D(&(gpb->gpb_WIN_ORG), 0.0, 0.0, 0.0);
	   Point_F3D(&(gpb->gpb_WIN_DIM), 1.0, 1.0, 1.0);
	  }
	gpb->gpb_flags |= GPB_WIN_FLAG;	/* make sure we recompute */
	break;
       }
}


/****
 *
 * xform_f3d(gpbobj, p, q)
 * Transform points P and Q (in viewport coordinates) into window coordinates
 * and put the result back into P and Q. If Q is null, only P is
 * transformed.
 *
 ****/
static
xform_f3d(gpbobj, p, a)
/*    --needs--			 */
	GPBUF	gpbobj;
FAST	POINT	*p, *a;
{
/*    --uses--			 */
local	void	gp_winview();	/* set window viewing transform */

FAST	GPDATA	*gpb;
FAST	PTR	ptr;
FAST	short	*done;		/* -> ready flag */
FAST	int	*first;
	short	*saveptr;
	POINT	q;
	int	x, y;		/* position of window */

	gpb = ObjAddr(gpbobj, GPDATA);
	gp_flush(gpb);				/* need fresh buffer */
	q = a ? *a : *p;
	saveptr = gpb->gpb_ptr;
	if (gpb->gpb_flags) gp_winview(gpb);	/* recalculate viewing? */
	ptr.sh = gpb->gpb_ptr;			/* where to put vector */
	if (gpb->gpb_attr.gpa_CLIPPLANES)	/* turn clipping off */
	   PUT_CMD(ptr.sh, GP1_SET_CLIP_PLANES, 0);
	PUT_CMD(ptr.sh, GP1_PROC_LINE_FLT_3D, 0);	/* transform vector list */
	PUT_SHORT(ptr.sh, 1);			/* 1 endpoint */
	PUT_SHORT(ptr.sh, 0);			/* clip flag = 0 */
	first = ptr.in;				/* -> first endpoint */
	PUT_FLOAT(ptr.fl, p->f.x);			/* copy floating endpoints */
	PUT_FLOAT(ptr.fl, p->f.y);
	PUT_FLOAT(ptr.fl, p->f.z);
	PUT_FLOAT(ptr.fl, q.f.x);
	PUT_FLOAT(ptr.fl, q.f.y);
	PUT_FLOAT(ptr.fl, q.f.z);
	done = ptr.sh;
	PUT_SHORT(ptr.sh, 1);
	if (gpb->gpb_attr.gpa_CLIPPLANES)	/* reset clipplanes */
	   PUT_CMD(ptr.sh, GP1_SET_CLIP_PLANES, gpb->gpb_attr.gpa_CLIPPLANES);
	PUT_CMD(ptr.sh, GP1_EOCL, 0);		/* end command list */
	gpb->gpb_ptr = ptr.sh;
#ifdef	DEBUG
	if (vpdbg) gp_print(gpb);		/* print debug info */
#endif
	POSTGPBUF(gpb, gpb->gpb_cmdofs);
	gp1_wait0(done, gpb->gpb_gfd);		/* wait for GP to finish */
	x = gpb->gpb_winx; y = gpb->gpb_winy;
	p->i.x = GET_INT(first) - x;		/* copy X coordinate */
	p->i.y = GET_INT(first) - y;		/* copy Y coordinate */
	p->i.z = 0; ++first;			/* skip Z */
	q.i.x = GET_INT(first) - x;		/* copy X coordinate */
	q.i.y = GET_INT(first) - y;		/* copy Y coordinate */
	q.i.z = 0;
#ifdef	DEBUG
	if (vpdbg)
	   printf("PROC_LINE_FLT_3D returns (%d, %d) (%d, %d)\n",
		p->i.x, p->i.y, q.i.x, q.i.y);
#endif
	if (a) *a = q;				/* update argument */
	gpb->gpb_ptr = saveptr;
	return (*((short *) first) - 1);
}

/****
 *
 * xform_f2d(gpbobj, p, q)
 * Transform points P and Q (in viewport coordinates) into window coordinates
 * and put the result back into P and Q. If Q is null, only P is
 * transformed.
 *
 ****/
static
xform_f2d(gpbobj, p, a)
/*    --needs--			 */
	GPBUF	gpbobj;
FAST	POINT	*p, *a;
{
/*    --uses--			 */
local	void	gp_winview();	/* set window viewing transform */

FAST	GPDATA	*gpb;
FAST	PTR	ptr;
FAST	short	*done;		/* -> ready flag */
FAST	int	*first;
	short	*saveptr;
	POINT	q;
	int	x, y;		/* position of window */

	gpb = ObjAddr(gpbobj, GPDATA);
	gp_flush(gpb);				/* need fresh buffer */
	q = a ? *a : *p;
	saveptr = gpb->gpb_ptr;
	if (gpb->gpb_flags) gp_winview(gpb);	/* recalculate viewing? */
	ptr.sh = gpb->gpb_ptr;			/* where to put vector */
	if (gpb->gpb_attr.gpa_CLIPPLANES)	/* turn clipping off */
	   PUT_CMD(ptr.sh, GP1_SET_CLIP_PLANES, 0);
	PUT_CMD(ptr.sh, GP1_PROC_LINE_FLT_2D, 0);	/* transform vector list */
	PUT_SHORT(ptr.sh, 1);			/* 1 endpoint */
	PUT_SHORT(ptr.sh, 0);			/* clip flag = 0 */
	first = ptr.in;				/* -> first endpoint */
	PUT_FLOAT(ptr.fl, p->f.x);			/* copy floating endpoints */
	PUT_FLOAT(ptr.fl, p->f.y);
	PUT_FLOAT(ptr.fl, q.f.x);
	PUT_FLOAT(ptr.fl, q.f.y);
	done = ptr.sh;
	PUT_SHORT(ptr.sh, 1);
	if (gpb->gpb_attr.gpa_CLIPPLANES)	/* reset clipplanes */
	   PUT_CMD(ptr.sh, GP1_SET_CLIP_PLANES, gpb->gpb_attr.gpa_CLIPPLANES);
	PUT_CMD(ptr.sh, GP1_EOCL, 0);		/* end command list */
	gpb->gpb_ptr = ptr.sh;
#ifdef	DEBUG
	if (vpdbg) gp_print(gpb);		/* print debug info */
#endif
	POSTGPBUF(gpb, gpb->gpb_cmdofs);
	gp1_wait0(done, gpb->gpb_gfd);		/* wait for GP to finish */
	x = gpb->gpb_winx; y = gpb->gpb_winy;
	p->i.x = GET_INT(first) - x;		/* copy X coordinate */
	p->i.y = GET_INT(first) - y;		/* copy Y coordinate */
	q.i.x = GET_INT(first) - x;		/* copy X coordinate */
	q.i.y = GET_INT(first) - y;		/* copy Y coordinate */
#ifdef	DEBUG
	if (vpdbg)
	   printf("PROC_LINE_FLT_2D returns (%d, %d) (%d, %d)\n",
		p->i.x, p->i.y, q.i.x, q.i.y);
#endif
	if (a) *a = q;				/* update argument */
	gpb->gpb_ptr = saveptr;
	return (*((short *) first) - 1);
}

/****
 *
 * xform_i3d(gpbobj, p, q)
 * Transform points P and Q (in viewport coordinates) into window coordinates
 * and put the result back into P and Q. If Q is null, only P is
 * transformed.
 *
 ****/
static
xform_i3d(gpbobj, p, a)
/*    --needs--			 */
	GPBUF	gpbobj;
FAST	POINT	*p, *a;
{
/*    --uses--			 */
local	void	gp_winview();	/* set window viewing transform */

FAST	GPDATA	*gpb;
FAST	PTR	ptr;
FAST	short	*done;		/* -> ready flag */
FAST	int	*first;
	short	*saveptr;
	POINT	q;
	int	x, y;		/* position of window */

	gpb = ObjAddr(gpbobj, GPDATA);
	gp_flush(gpb);				/* need fresh buffer */
	q = a ? *a : *p;
	saveptr = gpb->gpb_ptr;
	if (gpb->gpb_flags) gp_winview(gpb);	/* recalculate viewing? */
	ptr.sh = gpb->gpb_ptr;			/* where to put vector */
	if (gpb->gpb_attr.gpa_CLIPPLANES)	/* turn clipping off */
	   PUT_CMD(ptr.sh, GP1_SET_CLIP_PLANES, 0);
	PUT_CMD(ptr.sh, GP1_PROC_LINE_INT_3D, 0);	/* transform vector list */
	PUT_SHORT(ptr.sh, 1);			/* 1 endpoint */
	PUT_SHORT(ptr.sh, 0);			/* clip flag = 0 */
	first = ptr.in;				/* -> first endpoint */
	PUT_INT(ptr.in, p->i.x);			/* copy floating endpoints */
	PUT_INT(ptr.in, p->i.y);
	PUT_INT(ptr.in, p->i.z);
	PUT_INT(ptr.in, q.i.x);
	PUT_INT(ptr.in, q.i.y);
	PUT_INT(ptr.in, q.i.z);
	done = ptr.sh;
	PUT_SHORT(ptr.sh, 1);
	if (gpb->gpb_attr.gpa_CLIPPLANES)	/* reset clipplanes */
	   PUT_CMD(ptr.sh, GP1_SET_CLIP_PLANES, gpb->gpb_attr.gpa_CLIPPLANES);
	PUT_CMD(ptr.sh, GP1_EOCL, 0);		/* end command list */
	gpb->gpb_ptr = ptr.sh;
#ifdef	DEBUG
	if (vpdbg) gp_print(gpb);		/* print debug info */
#endif
	POSTGPBUF(gpb, gpb->gpb_cmdofs);
	gp1_wait0(done, gpb->gpb_gfd);		/* wait for GP to finish */
	x = gpb->gpb_winx; y = gpb->gpb_winy;
	p->i.x = GET_INT(first) - x;		/* copy X coordinate */
	p->i.y = GET_INT(first) - y;		/* copy Y coordinate */
	p->i.z = 0; ++first;			/* skip Z */
	q.i.x = GET_INT(first) - x;		/* copy X coordinate */
	q.i.y = GET_INT(first) - y;		/* copy Y coordinate */
	q.i.z = 0;
#ifdef	DEBUG
	if (vpdbg)
	   printf("PROC_LINE_INT_3D returns (%d, %d) (%d, %d)\n",
		p->i.x, p->i.y, q.i.x, q.i.y);
#endif
	if (a) *a = q;				/* update argument */
	gpb->gpb_ptr = saveptr;
	return (*((short *) first) - 1);
}

/****
 *
 * xform_i2d(gpbobj, p, q)
 * Transform points P and Q (in viewport coordinates) into window coordinates
 * and put the result back into P and Q. If Q is null, only P is
 * transformed.
 *
 ****/
static
xform_i2d(gpbobj, p, a)
/*    --needs--			 */
	GPBUF	gpbobj;
FAST	POINT	*p, *a;
{
/*    --uses--			 */
local	void	gp_winview();	/* set window viewing transform */

FAST	GPDATA	*gpb;
FAST	PTR	ptr;
FAST	short	*done;		/* -> ready flag */
FAST	int	*first;
	short	*saveptr;
	POINT	q;
	int	x, y;		/* position of window */

	gpb = ObjAddr(gpbobj, GPDATA);
	gp_flush(gpb);				/* need fresh buffer */
	q = a ? *a : *p;
	saveptr = gpb->gpb_ptr;
	if (gpb->gpb_flags) gp_winview(gpb);	/* recalculate viewing? */
	ptr.sh = gpb->gpb_ptr;			/* where to put vector */
	if (gpb->gpb_attr.gpa_CLIPPLANES)	/* turn clipping off */
	   PUT_CMD(ptr.sh, GP1_SET_CLIP_PLANES, 0);
	PUT_CMD(ptr.sh, GP1_PROC_LINE_INT_2D, 0);	/* transform vector list */
	PUT_SHORT(ptr.sh, 1);			/* 1 endpoint */
	PUT_SHORT(ptr.sh, 0);			/* clip flag = 0 */
	first = ptr.in;				/* -> first endpoint */
	PUT_INT(ptr.in, p->i.x);			/* copy floating endpoints */
	PUT_INT(ptr.in, p->i.y);
	PUT_INT(ptr.in, q.i.x);
	PUT_INT(ptr.in, q.i.y);
	done = ptr.sh;
	PUT_SHORT(ptr.sh, 1);
	if (gpb->gpb_attr.gpa_CLIPPLANES)	/* reset clipplanes */
	   PUT_CMD(ptr.sh, GP1_SET_CLIP_PLANES, gpb->gpb_attr.gpa_CLIPPLANES);
	PUT_CMD(ptr.sh, GP1_EOCL, 0);		/* end command list */
	gpb->gpb_ptr = ptr.sh;
#ifdef	DEBUG
	if (vpdbg) gp_print(gpb);		/* print debug info */
#endif
	POSTGPBUF(gpb, gpb->gpb_cmdofs);
	gp1_wait0(done, gpb->gpb_gfd);		/* wait for GP to finish */
	x = gpb->gpb_winx; y = gpb->gpb_winy;
	p->i.x = GET_INT(first) - x;		/* copy X coordinate */
	p->i.y = GET_INT(first) - y;		/* copy Y coordinate */
	q.i.x = GET_INT(first) - x;		/* copy X coordinate */
	q.i.y = GET_INT(first) - y;		/* copy Y coordinate */
#ifdef	DEBUG
	if (vpdbg)
	   printf("PROC_LINE_INT_2D returns (%d, %d) (%d, %d)\n",
		p->i.x, p->i.y, q.i.x, q.i.y);
#endif
	if (a) *a = q;				/* update argument */
	gpb->gpb_ptr = saveptr;
	return (*((short *) first) - 1);
}

/****
 *
 * gp_setview(gpb)
 * Computes mapping from user viewport to GP space. The GP coordinate
 * system has (-1,1,-1) in the upper left corner and (1,-1,1) in the
 * lower right corner (0,0,0) in the middle.
 *
 * VIEW_ORG = coordinates of upper left corner of user viewport
 * VIEW_DIM = dimensions (w/r to VIEW_ORG) of lower right corner
 *
 * view_trans = translation factor to map user viewport to GP space
 * view_scale = scaling factor to map user viewport to GP space
 *
 * VIEW_ORG-----+		    (-1,1,0)--------+
 *   |		|			|	    |
 *   |-------VIEW_ORG + VIEW_DIM        |---------(1,-1,1)
 * 
 *
 * If Pg is a point in GP space and Pv is the same point in the viewport
 *
 *	Pv = Pg * view_scale + view_trans
 *
 * Solving the following 2 equations for view_trans and view_scale:
 *	VIEW_ORG = (-1,1,0) * view_scale + view_trans
 *	VIEW_ORG + VIEW_DIM = (1,-1,1) * view_scale + view_trans
 *
 *	view_scale = (1/2, -1/2, 1) * VIEW_DIM
 *	view_trans = VIEW_ORG + (1/2, 1/2, 0) * VIEW_DIM
 *
 ****/
static
gp_setview(gpb)
/*    --needs--			 */
FAST	GPDATA	*gpb;
{
	switch (gpb->gpb_coord)
	  {
	   case VP_FLT3D:
	   gpb->gpb_view_scale.f.z = gpb->gpb_VIEW_DIM.f.z;
	   gpb->gpb_view_trans.f.z = gpb->gpb_VIEW_ORG.f.z;
	   goto flt2d;

	   case VP_FLT2D:
	   gpb->gpb_view_trans.f.z = 0.0;
	   gpb->gpb_view_scale.f.z = 1.0;
flt2d:	   gpb->gpb_view_scale.f.x = gpb->gpb_VIEW_DIM.f.x / 2;
	   gpb->gpb_view_scale.f.y = -gpb->gpb_VIEW_DIM.f.y / 2;
	   gpb->gpb_view_trans.f.x = gpb->gpb_VIEW_ORG.f.x +
		gpb->gpb_view_scale.f.x;
	   gpb->gpb_view_trans.f.y = gpb->gpb_VIEW_ORG.f.y -
		gpb->gpb_view_scale.f.y;
	   break;

	   case VP_INT3D:
	   gpb->gpb_view_scale.f.z = gpb->gpb_VIEW_DIM.i.z;
	   gpb->gpb_view_trans.f.z = gpb->gpb_VIEW_ORG.i.z;
	   goto int2d;

	   case VP_INT2D:
	   gpb->gpb_view_trans.f.z = 0.0;
	   gpb->gpb_view_scale.f.z = 1.0;
int2d:	   gpb->gpb_view_scale.f.x = gpb->gpb_VIEW_DIM.i.x / 2;
	   gpb->gpb_view_scale.f.y = -gpb->gpb_VIEW_DIM.i.y / 2;
	   gpb->gpb_view_trans.f.x = gpb->gpb_VIEW_ORG.i.x +
		gpb->gpb_view_scale.f.x;
	   gpb->gpb_view_trans.f.y = gpb->gpb_VIEW_ORG.i.y -
		gpb->gpb_view_scale.f.y;
	   break;
	  }
	gpb->gpb_flags &= ~GPB_VIEW_FLAG;
	gpb->gpb_flags |= GPB_XFORM_FLAG;
}

/****
 *
 * gp_setclip(gpb)
 * Computes mapping from user clip area to GP space. The GP coordinate
 * system has (-1,1,0) in the upper left corner and (1,-1,1) in the
 * lower right corner (0,0,0) in the middle.
 *
 * CLIP_ORG = coordinates of upper left corner of user clip area
 * CLIP_DIM = dimensions (w/r to CLIP_ORG) of lower right corner
 *
 * clip_org = origin of clip area in GP space
 * clip_dim = dimensions of clip area in GP space
 *
 * VIEW_ORG---------------+		    (-1,1,0)------------------+
 *   |			  |			|	    	      |
 *   | CLIP_ORG-------+   |                     | clip_org---------+  |
 *   |   |            |   |                     |  |               |  |
 *   |   |------CLIP_ORG + CLIP_DIM             |  +------clip_org * clip_dim
 *   |                    |                     |		      |
 *   |-------VIEW_ORG + VIEW_DIM		|-----------------(1,-1,1)
 * 
 *
 * If Pg is a point in GP space and Pv is the same point in the viewport
 *
 *	Pv = Pg * view_scale + view_trans
 *
 * Substituting the clip area coordinates and solving, we get:
 *
 *	CLIP_ORG = clip_org * view_scale + view_trans
 *	CLIP_ORG + CLIP_DIM = (clip_org + clip_dim) * view_scale + view_trans
 *
 *	clip_org = (CLIP_ORG - view_trans) / view_scale
 *	clip_dim = CLIP_DIM / view_scale
 *
 * Once we have the clip area coordinates relative to GP space (as
 * clip_org and clip_dim) rather than relative to the viewport (as
 * CLIP_ORG and CLIP_DIM), the mapping from viewport to GP clip area
 * does not change if the viewport coordinate system changes. The
 * viewport to clip area transformation matrix (see gp_view_xform)
 * does a further mapping of (clip_org, clip_dim) to the GP clip coordinates:
 *
 * (-1,1,0)---------------------        +------------------+
 *    |                         |       |                  |
 *    |  clip_org---------+     |       | (-1,1,0)-----+   |
 *    |    |              |     |       |   |          |   |
 *    |    +-------clip_org + clip_dim  |   |-----(1,-1,1) |
 *    |                         |       |                  |
 *    +--------------------(1,-1,1)     +------------------+
 *
 ****/
static
gp_setclip(gpb)
/*    --needs--			 */
FAST	GPDATA	*gpb;
{
	switch (gpb->gpb_coord)
	  {
	   case VP_FLT3D:
	   gpb->gpb_clip_org.f.z = (gpb->gpb_CLIP_ORG.f.z -
		gpb->gpb_view_trans.f.z) / gpb->gpb_view_scale.f.z;
	   gpb->gpb_clip_dim.f.z = gpb->gpb_CLIP_DIM.f.z /
		gpb->gpb_view_scale.f.z;
	   goto coflt;

	   case VP_FLT2D:
	   gpb->gpb_clip_org.f.z = 0.0;
	   gpb->gpb_clip_dim.f.z = 1.0;
coflt:	   gpb->gpb_clip_org.f.x = (gpb->gpb_CLIP_ORG.f.x -
		gpb->gpb_view_trans.f.x) / gpb->gpb_view_scale.f.x;
	   gpb->gpb_clip_org.f.y = (gpb->gpb_CLIP_ORG.f.y -
		gpb->gpb_view_trans.f.y) / gpb->gpb_view_scale.f.y;
	   gpb->gpb_clip_dim.f.x = gpb->gpb_CLIP_DIM.f.x /
		gpb->gpb_view_scale.f.x;
	   gpb->gpb_clip_dim.f.y = gpb->gpb_CLIP_DIM.f.y /
		gpb->gpb_view_scale.f.y;
	   break;

	   case VP_INT3D:
	   gpb->gpb_clip_org.f.z = (gpb->gpb_CLIP_ORG.i.z -
		gpb->gpb_view_trans.f.z) / gpb->gpb_view_scale.f.z;
	   gpb->gpb_clip_dim.f.z = gpb->gpb_CLIP_DIM.i.z /
		gpb->gpb_view_scale.f.z;
	   goto coint;

	   case VP_INT2D:
	   gpb->gpb_clip_org.f.z = 0.0;
	   gpb->gpb_clip_dim.f.z = 1.0;
coint:	   gpb->gpb_clip_org.f.x = (gpb->gpb_CLIP_ORG.i.x -
		gpb->gpb_view_trans.f.x) / gpb->gpb_view_scale.f.x;
	   gpb->gpb_clip_org.f.y = (gpb->gpb_CLIP_ORG.i.y -
		gpb->gpb_view_trans.f.y) / gpb->gpb_view_scale.f.y;
	   gpb->gpb_clip_dim.f.x = gpb->gpb_CLIP_DIM.i.x /
		gpb->gpb_view_scale.f.x;
	   gpb->gpb_clip_dim.f.y = gpb->gpb_CLIP_DIM.i.y /
		gpb->gpb_view_scale.f.y;
	   break;
	  }
	gpb->gpb_flags &= ~GPB_CLIP_FLAG;
	gpb->gpb_flags |= GPB_XFORM_FLAG;
}

/****
 *
 * gp_clearzbuf(gpb)
 * Append commands to set Z buffer (if VP_HIDDENSURF is set)
 * The Z buffer must be cleared every time the window position
 * or size changes, each time the screen is cleared.
 *
 * notes:
 * The SETZBUF command wants the physical screen area position and
 * dimensions on the GB. The clip list is applied to this area so only
 * the clip shape Z buffer will be cleared.
 *
 * uses:
 * gpb_winx, gpb_winy = physical coordinates of window
 * gpb_wdim = dimensions of window (in pixels)
 *
 * These are set by gp_winclip (and gp_init) when the clipping list
 * is loaded (every time the window size or position changes)
 *
 ****/
static void
gp_clearzbuf(gpb)
/*    --needs--			 */
FAST	GPDATA	*gpb;		/* -> GP buffer */
{
FAST	short	*ptr;

	if (gpb->gpb_attr.gpa_HIDDENSURF == 0) return;
	MAKEROOM(6 * sizeof(short));		/* enable and clear commands */
	ptr = gpb->gpb_ptr;
	PUT_CMD(ptr, GP1_SET_ZBUF, 0);		/* clear Z buffer */
	PUT_SHORT(ptr, -1);
	PUT_SHORT(ptr, gpb->gpb_winx);		/* physical window position */
	PUT_SHORT(ptr, gpb->gpb_winy);
	PUT_SHORT(ptr, gpb->gpb_wdim.r_width);	/* physical window dimensions */
	PUT_SHORT(ptr, gpb->gpb_wdim.r_height);
	gpb->gpb_ptr = ptr;
}

/****
 *
 * gp_init(gpb)
 * Initialize all GP parameters. Assumes empty buffer.
 *
 ****/
static void
gp_init(gpb)
/*    --needs--			 */
FAST	GPDATA	*gpb;
{
/*    --calls--			 */
extern	int	bcopy();

/*    --uses--			 */
static	uchar	cindex[] = { 0, 1 };
FAST	short	*ptr;
FAST	float	dx, dy;
	int	attr;

	win_getscreenposition(gpb->gpb_vp.vp_win->pw_windowfd,
		&(gpb->gpb_winx), &(gpb->gpb_winy));
	win_getsize(gpb->gpb_vp.vp_win->pw_windowfd, &(gpb->gpb_vp.vp_wdim));
	dx = gpb->gpb_wold.f.x = gpb->gpb_vp.vp_wdim.r_width;
	dy = gpb->gpb_wold.f.y = gpb->gpb_vp.vp_wdim.r_height;
	if (TypeInt(gpb->gpb_coord))
	  {
	   Point_I3D(&(gpb->gpb_VIEW_ORG), 0, 0, 0);
	   Point_I3D(&(gpb->gpb_VIEW_DIM), dx, dy, 1);
	   Point_I3D(&(gpb->gpb_CLIP_ORG), 0, 0, 0);
	   Point_I3D(&(gpb->gpb_CLIP_DIM), dx, dy, 1);
	  }
	else
	  {
	   Point_F3D(&(gpb->gpb_VIEW_ORG), 0.0, 0.0, 0.0);
	   Point_F3D(&(gpb->gpb_VIEW_DIM), dx, dy, 1.0);
	   Point_F3D(&(gpb->gpb_CLIP_ORG), 0.0, 0.0, 0.0);
	   Point_F3D(&(gpb->gpb_CLIP_DIM), dx, dy, 1.0);
	  }
	Point_F3D(&(gpb->gpb_WIN_ORG), 0.0, 0.0, 0.0);
	Point_F3D(&(gpb->gpb_WIN_DIM), 1.0, 1.0, 1.0);
	gpb->gpb_flags = GPB_XFORM_FLAG | GPB_WIN_FLAG |
		GPB_CLIP_FLAG | GPB_RESIZE_FLAG | GPB_VIEW_FLAG;
	bcopy(Identity3D, gpb->gpb_viewmatrix, sizeof(MATRIX));
	ptr = gpb->gpb_ptr;	
	PUT_CMD(ptr, GP1_SET_FB_NUM, 
	  gp1_d(gpb->gpb_vp.vp_win->pw_clipdata->pwcd_prmulti)->cg2_index);
	gpb->gpb_pixbits = gpb->gpb_pixmask = 1;
	gpb->gpb_cindex = cindex;
	PUT_CMD(ptr, GP1_SET_COLOR, gpb->gpb_color = 1);
	PUT_CMD(ptr, GP1_SET_ROP, 0);
	PUT_CMD(ptr, gpb->gpb_rop = PIX_SRC, 0);
	PUT_CMD(ptr, GP1_SET_FB_PLANES,
		gpb->gpb_fbplanes = gpb->gpb_pixplanes = 1);
	PUT_CMD(ptr, GP1_SET_CLIP_PLANES, 0);
	PUT_CMD(ptr, GP1_SET_HIDDEN_SURF, GP1_NOHIDDENSURF);
	PUT_CMD(ptr, GP1_SET_MAT_NUM, GPB_XFORMMATRIX);
	PUT_CMD(ptr, GP1_SET_LINE_WIDTH, 0);
	PUT_SHORT(ptr, 0); PUT_SHORT(ptr, 0);
	PUT_CMD(ptr, GP1_SET_LINE_TEX, 0);
	PUT_SHORT(ptr, 0); PUT_SHORT(ptr, 0); PUT_SHORT(ptr, 0);
	PUT_CMD(ptr, GP1_SET_PGON_TEX_ORG_SCR, 0);
	PUT_SHORT(ptr, 0); PUT_SHORT(ptr, 0);
	gpb->gpb_left -= (ptr - gpb->gpb_ptr) * sizeof(short);
	gpb->gpb_ptr = ptr;
	gp_matrix_set(gpb, GPB_XFORMMATRIX, Identity3D);
}

/****
 *
 * gp_winview(gpb)
 * Computes all viewing transformations
 *
 ****/
void
gp_winview(gpb)
/*    --needs--			 */
FAST	GPDATA	*gpb;
{
/*    --calls--			 */
local	void	gp_view_xform();
local	void	gp_win_xform();

	gp_view_xform(gpb);
	gp_win_xform(gpb);
}

/****
 *
 * gp_win_xform(gpb)
 * Compute transformation to map GP clip space to user window.
 * 
 * uses:
 *  WIN_ORG		Origin (upper left corner) of visible window area
 *  WIN_DIM		Dimensions of visible window area
 *  gpb_clip_org	Normalized origin of clip area (w/r to view area)
 *  gpb_clip_dim	Normalized dimensions of clip area (w/r to view area)
 *
 *  gpb_wdim		Dimensions (in pixels) of original window size
 *  gpb_left		number of bytes left in buffer
 *  gpb_ptr		-> next available byte
 *
 * notes:
 * There are several types of window scaling. If VP_AUTO_SCALE is not set,
 * the original size of the window is remembered (in gpb_wold) and, when
 * the window size changes, more or less of the picture is visible but
 * the image is not rescaled. If VP_AUTO_SCALE is set to VP_SCALE_ALL,
 * the image is scaled in both X and Y directions to map exactly onto the
 * portion of the window specified by WIN_ORG and WIN_DIM. If VP_SCALE_SQUARE
 * is used, a square aspect ratio is maintained. WIN_ORG and WIN_DIM are
 * automatically set to insure the image is centered and square each time
 * the window is resized.
 *
 * GP clip space has upper left corner at (-1,1,0) and lower right at (1,-1,1).
 * The window has (0,0,0) as its upper left and (Wdx, Wdy, 64K) as its
 * lower right (the Z value must be between 0 to 64K to use the Z buffer).
 *
 * The transformation matrix (gpb_viewmatrix) has mapped the viewport
 * clipping rectangle (CLIP_ORG : CLIP_DIM) onto the GP clip space
 * (-1,1,0) : (1,-1,1). To undo this, we conceptually have 2 transformations:
 *
 * 1) map GP clip space to normalized GP space: Pn = Pg * GPscale + GPtrans
 *    Solving for GPscale/GPtrans:
 *
 *	clip_org = (-1,1,0) * GPscale + GPtrans
 *	clip_org + clip_dim = (1,-1,1) * GPscale + GPtrans
 *
 *	GPscale = (1/2, -1/2, 1) * clip_dim
 *	GPtrans = clip_org + (1/2, 1/2, 0) * clip_dim
 *
 * (-1,1,0)------------+   clip_org--------------+ 
 *   |       Pg        |     |       Pn          |
 *   |                 |     |                   |
 *   +--------------(1,-1,1) +----------clip_org+clip_dim
 *
 * 2) map normalized GP space to the window:  Pw = Pn * Wscale + Wtrans
 *    Solving for Wscale/Wtrans:
 *
 *	(0,0,0) = (-1,1,0) * Wscale + Wtrans
 *	(Wdx, Wdy, 64K) = (1,-1,1) * Wscale + Wtrans
 *
 *	Wscale = (Wdx / 2, -Wdy / 2, 64K)
 *	Wtrans = (Wdx / 2, Wdy / 2, 0)
 *
 * (-1,1,0)----------------------+   (0,0,0)---------------------+
 *   |                      Pn   |      |                    Pw  |
 *   |  clip_org----------+      |      |  +---------------+     |
 *   |    |               |      |      |  |               |     | 
 *   |    +--------clip_org+clip_dim    |  +---------------+     |
 *   |                           |      |                        |
 *   +----------------------(1,-1,1)    +-------------------(Wdx, Wdy, 64K)
 *
 * Ideally, we would like to combine these transformations into one:
 *
 *	Pw = Pg * VWP_scale + VWP_trans
 *
 *   +---------------------------+   (0,0,0)---------------------+
 *   |                      Pg   |      |                    Pw  |
 *   | (-1,1,0)-----------+      |      |  +---------------+     |
 *   |    |               |      |      |  |               |     | 
 *   |    +------------(1,-1,1)  |      |  +---------------+     |
 *   |                           |      |                        |
 *   +---------------------------+      +-------------------(Wdx, Wdy, 64K)
 *
 *	Pw = (Pg * GPscale + GPtrans) * Wscale + Wtrans
 *	   = Pg * (GPscale * Wscale) + (GPtrans * Wscale + Wtrans)
 *
 *	VWP_scale = GPscale * Wscale =
 *		(Wdx / 4, Wdy / 4, 64K) * clip_dim
 *	VWP_trans = GPtrans * Wscale + Wtrans =
 *		(Wdx / 4, -Wdy / 4, 0) * clip_dim +
 *		(Wdx / 2, -Wdy / 2, 64K) + clip_org +
 *		(Wdx / 2, Wdy / 2, 0)
 *
 * These are the translation and scaling factors used with the SET_VWP_3D
 * command to map the GP clip space back to the window again.
 *
 ****/
static void
gp_win_xform(gpb)
/*    --needs--			 */
FAST	GPDATA	*gpb;
{
/*    --uses--			 */
FAST	PTR	ptr;
FAST	int	wfd;		/* window file descriptor */
	int	x, y;		/* window origin (physical coords) */
	float	dx, dy, dz;

	if (gpb->gpb_flags & (GPB_RESIZE_FLAG | GPB_WIN_FLAG))
	  {
	   MAKEROOM(4 * sizeof(float) + sizeof(short));
	   ptr.sh = gpb->gpb_ptr;
	   wfd = gpb->gpb_vp.vp_win->pw_clipdata->pwcd_windowfd;
	   win_getscreenposition(wfd, &x, &y);	/* get physical origin */
	   gpb->gpb_winx = x; gpb->gpb_winy = y;
/*
 * Window size changed? Handle scaling options
 * VP_AUTO_SCALE == 0	no scaling, use original window size
 *    VP_SCALE_ALL	scale to window in X, and Y
 *    VP_SCALE_SQUARE	maintain square aspect ratio
 */
	   if (gpb->gpb_flags & GPB_RESIZE_FLAG)
	     {
	      win_getsize(wfd, &(gpb->gpb_vp.vp_wdim));
	      if (gpb->gpb_attr.gpa_AUTO_SCALE)	/* don't automatically scale? */
	        {
	         gpb->gpb_wold.f.x = gpb->gpb_wdim.r_width;
	         gpb->gpb_wold.f.y = gpb->gpb_wdim.r_height;
/*
 * VP_SCALE_SQUARE
 * Preserve square aspect ration by setting WIN_ORG and WIN_DIM appropriately
 */
		 if (gpb->gpb_attr.gpa_AUTO_SCALE == VP_SCALE_SQUARE)
		   {
		    if (gpb->gpb_wdim.r_width > gpb->gpb_wdim.r_height)
		      {
		       Point_F3D(&(gpb->gpb_WIN_ORG), 0.5 *
			  (gpb->gpb_wdim.r_width - gpb->gpb_wdim.r_height) /
			  gpb->gpb_wdim.r_width, 0, 0);
		       Point_F3D(&(gpb->gpb_WIN_DIM),
			  (float) gpb->gpb_wdim.r_height /
			  gpb->gpb_wdim.r_width, 1, 1);
		      }
		    else
		      {
		       Point_F3D(&(gpb->gpb_WIN_ORG), 0, 0.5 *
			  (gpb->gpb_wdim.r_height - gpb->gpb_wdim.r_width) /
			  gpb->gpb_wdim.r_height, 0);
		       Point_F3D(&(gpb->gpb_WIN_DIM), 1,
			  (float) gpb->gpb_wdim.r_width /
			  gpb->gpb_wdim.r_height, 1);
		      }
		   }
	        }
	     }
/*
 * Calculate mapping from GP clip area to window
 * VWP_scale = (Wdx / 4, Wdy / 4, 64K) * clip_dim
 * VWP_trans = (Wdx / 4, -Wdy / 4, 0) * clip_dim +
 *		(Wdx / 2, -Wdy / 2, 64K) + clip_org + (Wdx / 2, Wdy / 2, 0)
 *
 * (dx, dy, dz) = (Wdx / 4, Wdy / 4, 64K) * clip_dim
 * (x, y, z) = (dx, dy, dz) * ((1, -1, 0) +
 *	((2, -2, 1) * clip_org + (2, 2, 0)) / clip_dim)
 */
	   dz = 0XFFFF * gpb->gpb_clip_dim.f.z;
	   dx = (gpb->gpb_wold.f.x * gpb->gpb_WIN_DIM.f.x) *
	   	gpb->gpb_clip_dim.f.x / 4.0;
	   dy = (gpb->gpb_wold.f.y * gpb->gpb_WIN_DIM.f.y) *
	   	gpb->gpb_clip_dim.f.y / 4.0;
	   x += dx * (1.0 + ((2.0 * gpb->gpb_clip_org.f.x + 2.0) /
	   	gpb->gpb_clip_dim.f.x));
	   y += dy * (-1.0 + ((-2.0 * gpb->gpb_clip_org.f.y + 2.0) /
	   	gpb->gpb_clip_dim.f.y));
	   PUT_CMD(ptr.sh,  GP1_SET_VWP_3D, 0);
	   PUT_FLOAT(ptr.fl, dx);
	   PUT_FLOAT(ptr.fl, x + gpb->gpb_wold.f.x * gpb->gpb_WIN_ORG.f.x);
	   PUT_FLOAT(ptr.fl, dy);
	   PUT_FLOAT(ptr.fl, y + gpb->gpb_wold.f.y * gpb->gpb_WIN_ORG.f.y);
	   PUT_FLOAT(ptr.fl, dz);
	   PUT_FLOAT(ptr.fl, dz * gpb->gpb_clip_org.f.z / gpb->gpb_clip_dim.f.z);
	   gpb->gpb_ptr = ptr.sh;
	  }
	gpb->gpb_flags &= ~(GPB_RESIZE_FLAG | GPB_WIN_FLAG);
}

/****
 *
 * gp_view_xform(gpb)
 * Compute viewing transformation matrix which maps the user viewport
 * (VIEW_ORG, VIEW_DIM) onto the GP clip area. The GP clips to a pyramid
 * with upper left corner at (-1,1,-1) and lower right corner (1,-1,1).
 * 
 * uses:
 *  VIEW_ORG		Origin (upper left corner) of viewport
 *  VIEW_DIM		Dimensions of viewport
 *  CLIP_ORG		Origin (upper left corner) of clip area
 *  CLIP_DIM		Dimensions of clip area
 *
 *  gpb_view_trans	Translation factors for viewport to GP mapping
 *  gpb_view_scale	Scaling factors for viewport to GP mapping
 *  gpb_clip_org	Normalized center of clip area (w/r to view area)
 *  gpb_clip_dim	Normalized dimensions of clip area (w/r to view area)
 *  gpb_left		number of bytes left in buffer
 *  gpb_ptr		-> next available byte
 *
 * notes:
 * VIEW_ORG = coordinates of upper left corner of user viewport
 * VIEW_DIM = dimensions (w/r to VIEW_ORG) of lower right corner
 * CLIP_ORG = coordinates of upper left corner of viewport clip area
 * CLIP_DIM = dimensions (w/r to CLIP_ORG) of lower right corner
 *
 * view_trans = translation factor to map user viewport to GP clip area
 * view_scale = scaling factor to map user viewport to GP clip area
 * clip_org = normalized coordinates of clip origin in GP space
 * clip_dim = normalized dimensions of clip area in GP space
 *
 * The GP clip area has upper left corner at (-1, 1, 0) and
 * lower right at (1, -1, 1). If Pg is a point in the GP clip area and Pv
 * is the corresponding point in the viewport:
 *
 *	Pv = Pg * Vscale + Vtrans 
 *
 * VIEW_ORG------------------------+    +---------------------------+
 *   |                         Pv  |    |                       Pg  |
 *   |  CLIP_ORG----------+        |    |  (-1,1,0)-----------+     |
 *   |   |                |        |    |    |                |     | 
 *   |   +-----CLIP_ORG + CLIP_DIM |    |    +-----------(1,-1,1)   |
 *   |                             |    |                           |
 *   +------------VIEW_ORG + VIEW_DIM   +---------------------------+
 *
 * This transformation can be broken down into 2 other transformations
 * to put it in terms of quantities we have already computed:
 *
 * 1) map viewport to normalized GP space
 *    (view_scale and view_trans have already been computed by gp_setview)
 *    Given that, Pv = Pn * view_scale + view_trans, the reverse transform is:
 *
 *	Pn = (1 / view_scale) * Pv - view_trans / view_scale
 *    
 *
 * VIEW_ORG------------------------+  (-1,1,0)------------------------+
 *   |                         Pv  |    |                       Pn    |
 *   |  CLIP_ORG----------+        |    | clip_org------------+       |
 *   |   |                |        |    |    |                |       |
 *   |   +-----CLIP_ORG + CLIP_DIM |    |    +----clip_org + clip_dim |
 *   |                             |    |                             |
 *   +------------VIEW_ORG + VIEW_DIM   +------------------------(1,-1,1)
 *
 * 2) map normalized GP space to GP clip space: Pg = Pn * GPscale + GPtrans
 *    Solving for GPscale/GPtrans:
 *
 *	(-1, 1, 0) = clip_org * GPscale + GPtrans
 *	(1, -1, 1) = (clip_org + clip_dim) * GPscale + GPtrans
 *
 *	GPscale = (2, -2, 1) / clip_dim
 *	GPtrans = (-1, 1, 0) + (-2, 2, -1) * clip_org / clip_dim
 *
 * (-1,1,0)----------------------+      +------------------------+
 *   |                      Pn   |      |                    Pg  |
 *   |  clip_org----------+      |      | (-1,1,0)---------+     |
 *   |    |               |      |      |  |               |     | 
 *   |    +------clip_org + clip_dim    |  +------------(1,-1,1) |
 *   |                           |      |                        |
 *   +----------------------(1,-1,1)    +------------------------+
 *
 *
 * Pg = Pv * Vscale + Vtrans
 * Pg = Pn * GPscale + GPtrans =
 *      (Pv / view_scale - view_trans / view_scale) * GPscale + GPtrans
 *
 * Vscale = GPscale / view_scale = (2, -2, 1) / (view_scale * clip_dim)
 * Vtrans = (-view_trans / view_scale) * GPscale + GPtrans =
 *	  = -view_trans * Vscale +
 *	    (-1, 1, 0) + (-2, 2, -1) * clip_org / clip_dim
 *
 * viewmatrix looks like:
 *
 *	Vscalex     0        0      0
 * 	 0	 Vscaley     0      0
 * 	 0	    0     Vscalez   0
 * 	Vtransx  Vtransy  Vtransz   1
 *
 ****/
static void
gp_view_xform(gpb)
/*    --needs--			 */
FAST	GPDATA	*gpb;
{
/*    --calls--			 */
local	int	gp_setclip();	/* compute clip_org / clip_dim */
local	int	gp_setview();	/* compute view_trans / view_scale */

	if (gpb->gpb_flags & GPB_VIEW_FLAG) gp_setview(gpb);
	if (gpb->gpb_flags & GPB_CLIP_FLAG) gp_setclip(gpb);
	if (gpb->gpb_flags & GPB_XFORM_FLAG)
	  {
	   gpb->gpb_viewmatrix[0][0] = 2.0 / (gpb->gpb_view_scale.f.x *
	   	gpb->gpb_clip_dim.f.x);
	   gpb->gpb_viewmatrix[1][1] = -2.0 / (gpb->gpb_view_scale.f.y *
	   	gpb->gpb_clip_dim.f.y);
	   gpb->gpb_viewmatrix[2][2] = 1.0 / (gpb->gpb_view_scale.f.z *
	   	gpb->gpb_clip_dim.f.z);
	   gpb->gpb_viewmatrix[3][0] = -1.0 -
		gpb->gpb_view_trans.f.x * gpb->gpb_viewmatrix[0][0] -
		2.0 * gpb->gpb_clip_org.f.x / gpb->gpb_clip_dim.f.x;
	   gpb->gpb_viewmatrix[3][1] = 1.0 -
		gpb->gpb_view_trans.f.y * gpb->gpb_viewmatrix[1][1] +
		2.0 * gpb->gpb_clip_org.f.y / gpb->gpb_clip_dim.f.y;
	   gpb->gpb_viewmatrix[3][2] = 
		-gpb->gpb_view_trans.f.z * gpb->gpb_viewmatrix[2][2] -
		gpb->gpb_clip_org.f.z / gpb->gpb_clip_dim.f.z;
	   gp_matrix_set(gpb, GPB_VIEWMATRIX, gpb->gpb_viewmatrix);
	   if (gpb->gpb_attr.gpa_MATRIX)
	      gp_matrix_mul(gpb, gpb->gpb_attr.gpa_MATRIX,
	   	GPB_VIEWMATRIX, GPB_XFORMMATRIX);
	   else gp_matrix_set(gpb, GPB_XFORMMATRIX, gpb->gpb_viewmatrix);
	  }
	gpb->gpb_flags &= ~GPB_XFORM_FLAG;
}

/****
 *
 * gp_winclip
 * Compute clipping list for graphics subwindow.
 * This copies the clip list into the GP buffer so that all operations
 * will clip to the window. If VP_HIDDENSURF is enabled, the Z buffer
 * is also cleared.
 *
 * notes:
 * Does no checking to see if there is room in the buffer.
 * This is only designed to be called right before a flush/post.
 * gpb_ptr is trashed - do not depend on it!!
 *
 ****/
static void
gp_winclip(gpb)
/*    --needs--			 */
FAST	GPDATA	*gpb;		/* -> GP buffer */
{
/*    --uses--			 */
FAST	struct gp1pr *dmd;
FAST	PTR	ptr;
FAST	struct pixwin_prlist *prl;
FAST	short	*rectptr;
FAST	int	n;

/*
 * copy each clipping rectangle into GP buffer
 */
	ptr.sh = gpb->gpb_ptr;
	PUT_CMD(ptr.sh, GP1_SET_CLIP_LIST, 0);
	rectptr = ptr.sh++;
	prl = gpb->gpb_vp.vp_win->pw_clipdata->pwcd_prl;
	n = 0;
	while (prl && (n < GPB_MAXCLIP))	/* another rectangle? */
	  {
	   ++n;
	   dmd = gp1_d(prl->prl_pixrect);	/* get pixrect */
	   PUT_SHORT(ptr.sh, dmd->cgpr_offset.x);	/* X, Y origin of rect */
	   PUT_SHORT(ptr.sh, dmd->cgpr_offset.y);
	   PUT_SHORT(ptr.sh, prl->prl_pixrect->pr_size.x); /* X, Y size of rect */
	   PUT_SHORT(ptr.sh, prl->prl_pixrect->pr_size.y);
	   prl = prl->prl_next;			/* go to next one */
	  }
	*rectptr = n;
	gpb->gpb_ptr = ptr.sh;
	PUT_CMD(gpb->gpb_ptr, GP1_EOCL, GP1_FREEBLKS);
	PUT_INT(gpb->gpb_ptr, gpb->gpb_cmdbv);
#ifdef	DEBUG
	if (vpdbg > 0)
          {
	   printf("Window Viewing Commands\n");
	   gp_print(gpb);
          }
#endif
	POSTGPBUF(gpb, gpb->gpb_cmdofs);
}

/****
 *
 * gp_allocbuf(gpb)
 * Allocate GP buffer
 * 1. allocate buffer
 * 2. initialize pointers and size
 * 3. indicate which status block to use
 *
 *	gpb_size	block size of command buffer
 *	gpb_vstart	vector list start code (0)
 *	gpb_cmdofs	command buffer shared memory block offset
 *	gpb_cmdbv	command buffer bit vector of blocks to free
 *	gpb_left	bytes in buffer (gpb_size * GPB_BYTESPERBLOCK)
 *	gpb_ptr		-> command buffer
 *			GP1_USE_CONTEXT, status block index
 *
 ****/
static void
gp_allocbuf(gpb)
/*    --needs--			 */
FAST	GPDATA	*gpb;
{
	while ((gpb->gpb_cmdofs = gp1_alloc(gpb->gpb_shmem,
			gpb->gpb_size, &(gpb->gpb_cmdbv),
			gpb->gpb_mindev, gpb->gpb_gfd)) == 0)
	   ;
	gpb->gpb_ptr = &((short *) gpb->gpb_shmem)[gpb->gpb_cmdofs];
	PUT_CMD(gpb->gpb_ptr, GP1_USE_CONTEXT, gpb->gpb_sbindex & 0x7);
	gpb->gpb_vstart = 0;
	gpb->gpb_left = gpb->gpb_size * GPB_BYTESPERBLOCK - 8;
}

/****
 *
 * gp_start(gpb)
 * Start list of vectors, polygon, etc. Valid opcodes are:
 *	GP1_XF_LINE_INT_2D, GP1_XF_LINE_INT_3D,
 *	GP1_XF_LINE_FLT_2D, GP1_XF_LINE_FLT_3D,
 *	GP1_XF_PGON_INT_2D, GP1_XF_PGON_INT_3D,
 *	GP1_XF_PGON_FLT_2D, GP1_XF_PGON_FLT_3D
 *
 * Vector list looks like:
 * 	0	GP1_XF_LINE_xxx_3D
 *	2	move/draw x1, y1, z1 (vector 1)
 *	18	move/draw x2, y2, z2 1(vector 2)
 *		...
 8
 * Polygon list looks like:
 *	0	GP1_XF_PGON_xxx_3D
 *	2	# of polygons
 *	4	# of edges for first polygon (each edge is one point)
 *	6	# of edges for 2nd polygon
 *		...
 *		x1, y1, z1 (edge 1) (first polygon)
 *		x2, y2, z2 (edge 2)
 *		...
 *		x1, y1, z2 (edge 1) (2nd polygon)
 *		...
 *
 * notes:
 * gpb_vecsize points to the word in the GP command buffer which
 * has the number of vectors stored. gpb_polysize points to the word
 * with the number of polygons stored.
 *
 ****/
void
gp_start(gpb)
/*    --needs--			 */
FAST	GPDATA	*gpb;
{
/*    --uses--			 */
local	void	gp_winview();	/* set window viewing transform */

local	short	startypes[];
FAST	PTR	ptr;

	if (gpb->gpb_flags) gp_winview(gpb);	/* recalculate viewing? */
	if (gpb->gpb_ftype & GPB_POLYMASK)	/* start polygon? */
	  {
	   gp_flush(gpb);
	   gpb->gpb_vstart = startypes[gpb->gpb_ftype];
	   if (gpb->gpb_fill == VP_SHADE_GOURAUD)
	      gpb->gpb_vstart |= GP1_SHADE_GOURAUD;
	   else if (gpb->gpb_pattern) gpb->gpb_vstart |= GP1_SHADE_TEX;
	   PUT_CMD(gpb->gpb_ptr,  gpb->gpb_vstart, 0);
	   gpb->gpb_polysize = gpb->gpb_ptr;	/* polygon list size */
	   PUT_SHORT(gpb->gpb_ptr, 1);
	   gpb->gpb_vecsize = gpb->gpb_ptr;
	   PUT_SHORT(gpb->gpb_ptr, 0);
	   USEUP(3 * sizeof(short));
	  }
	else					/* start vector */
	  {
	   if (!ROOMFOR(4 * sizeof(short) + 6 * sizeof(float)))
	      gp_flush(gpb);
	   PUT_CMD(gpb->gpb_ptr,
		gpb->gpb_vstart = startypes[gpb->gpb_ftype], 0);
	   USEUP(sizeof(short));
	   gpb->gpb_polysize = 0;
	  }
}

/****
 *
 * gp_poly_bound(gpb)
 * Make room for a new bound for the current polygon. The polygon data
 * must be shuffled down one word so the new bound's vector count can
 * be inserted properly.
 *
 * returns:
 *   0 - not enough room in buffer for another bound
 *   nonzero - successful
 *
 * notes:
 * gpb_polysize -> polygon list size word. This is the word after
 *	the start of the polygon command.
 * gpb_vecsize -> vector list size word for current bound. The new
 *	size word goes right after this one.
 *
 ****/
static int
gp_poly_bound(gpb)
/*    --needs--			 */
FAST	GPDATA	*gpb;
{
/*    --uses--			 */
FAST	short	*from, *to;
FAST	int	n;

#if	0
	if ((gpb->gpb_fill == VP_SHADE_GOURAUD) &&
	    (gpb->gpb_ftype & GPB_POLYMASK))	/* start polygon? */
	  {
	   if (!ROOMFOR((3*sizeof(short)) + (16*sizeof(float))))
	     gp_flush(gpb);
	   gpb->gpb_vstart = startypes[gpb->gpb_ftype];
	   gpb->gpb_vstart |= GP1_SHADE_GOURAUD;
	   PUT_CMD(gpb->gpb_ptr,  gpb->gpb_vstart, 0);
	   gpb->gpb_polysize = gpb->gpb_ptr;	/* polygon list size */
	   PUT_SHORT(gpb->gpb_ptr, 1);
	   gpb->gpb_vecsize = gpb->gpb_ptr;
	   PUT_SHORT(gpb->gpb_ptr, 0);
	   USEUP(3 * sizeof(short));
	   return (1);
	  }
#endif
	if (!ROOMFOR(sizeof(short) + (9 * sizeof(float))))
	  {
	   gpb->gpb_vstart = 0;			/* just close polygon */
	   return (0);				/* say it won't fit */
	  }
	USEUP(sizeof(short));
	to = (gpb->gpb_ptr)++;			/* end of polygon data */
	from = to - 1;				/* where to shuffle from */
	n = to - ++(gpb->gpb_vecsize);		/* how many words to shuffle */
	while (--n >= 0) *to-- = *from--;	/* shuffle it down */
	*to = 0;				/* # vectors in this bound */
	++(*(gpb->gpb_polysize));		/* count this bound */
	return (1);
}

/****
 *
 * gp_load_pat(gpb, bmobj)
 * Loads a polygon pattern into the GP. The pattern is in a BITMAP.
 * If BMOBJ is NULL, solid polygons are drawn.
 *
 ****/
gp_load_pat(gpb, bmobj)
/*    --needs--			 */
FAST	GPDATA	*gpb;		/* -> GP structure info */
FAST	OBJID	bmobj;		/* bitmap object */
{
FAST	PTR	ptr;
	int	n, size;

	ptr.sh = gpb->gpb_ptr;
	if (bmobj)				/* use a pattern? */
	  {
	   if (gpb->gpb_pat_block <= 0)		/* no texture block? */
	     {
	      if ((n = gp1_get_static_block(gpb->gpb_gfd)) < 0)
		 error("Set_VP: cannot set polygon pattern\n", 0, 0);
	      MAKEROOM(2 * sizeof(short));
	      ptr.sh = gpb->gpb_ptr;
	      gpb->gpb_pat_block = n;		/* polygon texture */
	      PUT_CMD(ptr.sh, GP1_SET_PGON_TEX_BLK, n);
	     }
/*
 * Load the texture into the GP.
 * We figure out the size of the texture in bytes and then copy its
 * data into the GP command buffer. Load_Bitmap uses the same format
 * as the GP wants for textures.
 */
	   size = Get_Bitmap(bmobj, BMAP_SIZE);	/* size in bytes */
	   MAKEROOM(4 * sizeof(short) + size);
	   ptr.sh = gpb->gpb_ptr;
	   PUT_CMD(ptr.sh, GP1_SET_PGON_TEX, 0);	/* set texture command */
	   PUT_SHORT(ptr.sh, Get_Bitmap(bmobj, BMAP_DEPTH));
	   PUT_SHORT(ptr.sh, Get_Bitmap(bmobj, BMAP_WIDTH));
	   PUT_SHORT(ptr.sh, Get_Bitmap(bmobj, BMAP_HEIGHT));
	   Load_Bitmap(bmobj, ptr.ch);		/* copy texture */
	   ptr.ch += size;			/* skip over it */
	   gpb->gpb_ptr = ptr.sh;
	  }
}

/****
 *
 * print_gp(gpbobj)
 * Print contents of GP buffer (commands)
 *	
 ****/
#define	GPOP_MAX	62

print_gp(gpbobj) GPBUF gpbobj; { gp_print(ObjAddr(gpbobj, GPDATA)); }

gp_print(gpb)
/*    --needs--			 */
FAST	GPDATA	*gpb;		/* -> GP buffer */
{
FAST	PTR	ptr;
FAST	int	n;
FAST	uchar	c;
static	string	gpopcodes[] = {
	"EOCL",		"USE_CONTEXT",	"PR_VEC",	"PR_ROP_NF",
	"PR_ROP_FF",	"PR_PGON_SOL",	"SET_ZBUF",	"SET_HIDDEN_SURF",
	"SET_MAT_NUM",	"MUL_POINT_FLT_2D",	"MUL_POINT_FLT_3D",
	"XF_PGON_FLT_3D",	"XF_PGON_FLT_2D", "corendcvec_3d",
	"cgivec",	"SET_CLIP_LIST",	"SET_FB_NUM",
	"SET_VWP_3D",	"SET_VWP_2D",	"SET_ROP", "SET_CLIP_PLANES",
	"MUL_POINT_INT_2D",	"MUL_POINT_INT_3D",	"SET_FB_PLANES",
	"SET_MAT_3D",	"xfvec_3d",	"passthru",	"xfvec_2d",
	"SET_COLOR",	"SET_MAT_2D",	"XF_PGON_INT_3D", "zoom",
	"MUL_MAT_2D",	"MUL_MAT_3D",	"GET_MAT_2D",	"GET_MAT_3D",
	"PROC_LINE_FLT_3D", "PROC_PGON_FLT_3D", "op38", "SET_EF_TEX",
	"PR_LINE",	"PR_POLYLINE",	"SET_LINE_TEX",	"SET_LINE_WIDTH",
	"CGI_LINE",	"XF_LINE_FLT_2D", "XF_LINE_FLT_3D", "XF_LINE_INT_3D",
	"PR_PGON_TEX",	"PR_ROP_TEX1",	"PR_ROP_TEX8",	"SET_PGON_TEX_BLK",
	"SET_PGON_TEX",	"SET_PGON_TEX_ORG_SCR",
	"SET_PGON_TEX_ORG_XF_2D",	"SET_PGON_TEX_ORG_XF_3D",
	"SET_BKGND_COLOR", "XF_LINE_INT_2D", "XF_PGON_INT_2D",
	"59?",	"PROC_LINE_FLT_2D", "PROC_LINE_INT_2D", "PROC_LINE_INT_3D",
	};

	if (gpb->gpb_ptr == NULL) return;
	ptr.sh = &((short *) gpb->gpb_shmem)[gpb->gpb_cmdofs];
	while (ptr.sh < gpb->gpb_ptr)		/* til buffer end */
	  {
	   if ((c = *ptr.ch) > GPOP_MAX)	/* illegal opcode */
	     {
	      printf("? %4.4X\n", (ushort) GET_SHORT(ptr.sh));
	      continue;
	     }
	   printf("%s", gpopcodes[c]);
	   c = *(ptr.ch + 1);
	   switch (GET_SHORT(ptr.sh) & 0XFF00)
	     {
	      default:				/* opcode, no args */
	      printf(" | 0x%x\n", (uchar) c);
	      break;

/*
 * EOCL | [ FREEBLKS ]
 * (int) bit vector of blocks
 */
	      case GP1_EOCL:			/* end it all */
	      if (c == GP1_FREEBLKS)
	         printf("\t| GP1_FREEBLKS, BV = 0%4.4x\n", GET_INT(ptr.in));
	      else printf("\n");
	      return;

/*
 * SET_ROP
 * (short) pixrect style raster op
 */
	      case GP1_SET_ROP:
	      printf("\t%4.4x\n", (ushort) GET_SHORT(ptr.sh));
	      break;

/*
 * SET_LINE_WIDTH width, 0
 */
	      case GP1_SET_LINE_WIDTH:
	      printf("\t%d\n", GET_SHORT(ptr.sh));
	      ptr.sh++;
	      break;

/*
 * SET_LINE_TEX pattern options offset
 */
	      case GP1_SET_LINE_TEX:
	     {
	      FAST int i;

	      i = 0;
	      while (n = GET_SHORT(ptr.sh))	/* another texture word? */
		{
		 if (--i < 0)			/* 8 columns? */
		   {
		    printf("\n");		/* start another line */
		    i = 7;
		   }
		 printf("  %4.4x", (ushort) n);
		}
	      n = GET_SHORT(ptr.sh);
	      printf("\n  options = %d, offset = %d\n", n, GET_SHORT(ptr.sh));
	     }
	      break;

/*
 * SET_EF_TEX <16 word texture>
 */
	      case GP1_SET_EF_TEX:
	     {
	      FAST int i, j;

	      printf("\n");
	      for (j = 0; ++j <= 4;)
		{
	         for (i = 0; ++i <= 4;)
		    printf("  %4.4x", (ushort) GET_SHORT(ptr.sh));
		 printf("\n");
		}
	     }
	      break;

/*
 * SET_PGON_TEX_ORG_SCR sx, sy
 * SET_PGON_TEX_ORG_XF_2D sx, sy
 * SET_PGON_TEX_ORG_XF_3D sx, sy, sz
 * SET_PGON_TEX depth, width, height, pattern
 */
	      case GP1_SET_PGON_TEX_ORG_SCR:
	      n = GET_SHORT(ptr.sh);
	      printf(" %d, %d\n", n, GET_SHORT(ptr.sh));
	      break;

	      case GP1_SET_PGON_TEX_ORG_XF_2D:
 	     {
	      float f;

	      f = GET_FLOAT(ptr.fl);
	      printf(" %d, %d\n", f, GET_FLOAT(ptr.fl));
	      break;
	     }

	      case GP1_SET_PGON_TEX_ORG_XF_3D:
 	     {
	      float f1, f2;

	      f1 = GET_FLOAT(ptr.fl);
	      f2 = GET_FLOAT(ptr.fl);
	      printf(" %d, %d, %d\n", f1, f2, GET_FLOAT(ptr.fl));
	      break;
	     }

	      case GP1_SET_PGON_TEX:
	     {
	      FAST int i, s, d, w, h;

	      d = GET_SHORT(ptr.sh); w = GET_SHORT(ptr.sh);
	      h = GET_SHORT(ptr.sh);
	      printf(" depth=%d, width=%d, height=%d", d, w, h);
	      s = (d == 1) ? h * ((w + 15) >> 4) : (h * w + 1) >> 1;
	      i = 0;
	      while (--s >= 0)			/* for each texture word */
		{
		 if (--i < 0)			/* 8 columns? */
		   {
		    printf("\n");		/* start another line */
		    i = 7;
		   }
		 printf("  %4.4x", (ushort) GET_SHORT(ptr.sh));
		}
	      printf("\n");
	     }
	      break;

/*
 * PR_ROP_NF | pixel planes mask
 * (short) frame buffer index
 * (short) raster op / color
 * clip rectangle (in device coordinates)
 *	(short) x origin of upper left corner
 *	(short) y origin of upper left corner
 *	(short) width in pixels
 *	(short) height in pixels
 * destination rectangle (with respect to clip rectangle)
 *	(short) x origin of upper left corner
 *	(short) y origin of upper left corner
 *	(short) width in pixels
 *	(short) height in pixels
 */
	      case GP1_PR_ROP_NF:
	     {
	      int x, y, dx, dy;

	      n = GET_SHORT(ptr.sh);
	      printf(" PIXP = 0%x FBI = %d, ROP/COLOR = 0%x\n",
			c, n, GET_SHORT(ptr.sh));
	      x = GET_SHORT(ptr.sh); y = GET_SHORT(ptr.sh);
	      dx = GET_SHORT(ptr.sh); dy = GET_SHORT(ptr.sh);
	      printf("\tCLIP: %d, %d  %d x %d\n", x, y, dx, dy);
	      x = GET_SHORT(ptr.sh); y = GET_SHORT(ptr.sh);
	      dx = GET_SHORT(ptr.sh); dy = GET_SHORT(ptr.sh);
	      printf("\tDEST: %d, %d  %d x %d\n", x, y, dx, dy);
	      break;
	     }

/*
 * MUL_MAT_2D / MUL_MAT_3D
 * (short) matrix A
 * (short) matrix B
 * (short) matrix C
 */
	      case GP1_MUL_MAT_2D:
	      case GP1_MUL_MAT_3D:
	     {
	      int a, b, c;

	      a = GET_SHORT(ptr.sh); b = GET_SHORT(ptr.sh); c = GET_SHORT(ptr.sh);
	      printf("\t%d, %d, %d\n", a, b, c);
	      break;
	     }

/*
 * SET_MAT_2D | matrix
 * (float) Mat[1,1]
 * (float) Mat[1,2]
 * (float) Mat[2,1]
 *	...
 * (float) Mat[3,2]
 */
	      case GP1_SET_MAT_2D:		/* matrix should follow */
	      printf(" | %d\n", (uchar) c);
mat2d:	     {
	      float m1, m2, m3, m4, m5, m6;

	      m1 = GET_FLOAT(ptr.sh); m2 = GET_FLOAT(ptr.sh); m3 = GET_FLOAT(ptr.sh);
	      m4 = GET_FLOAT(ptr.sh); m5 = GET_FLOAT(ptr.sh); m6 = GET_FLOAT(ptr.sh);
	      printf("\t%f\t%f\n\t%f\t%f\n\t%f\t%f\n", m1, m2, m3, m4, m5, m6);
	      break;
	     }

/*
 * SET_MAT_3D | matrix
 * (float) Mat[1,1]
 * (float) Mat[1,2]
 *	...
 * (float) Mat[4,4]
 */
	      case GP1_SET_MAT_3D:		/* matrix should follow */
	      printf(" | %d\n", (uchar) c);
mat3d:	      n = 0;
	     {
	      float m1, m2, m3, m4;

	      while (++n <= 4)			/* 4 x 4 matrix */
		{
	         m1 = GET_FLOAT(ptr.sh); m2 = GET_FLOAT(ptr.fl);
	         m3 = GET_FLOAT(ptr.sh); m4 = GET_FLOAT(ptr.fl);
		 printf("\t%f\t%f\t%f\t%f\n", m1, m2, m3, m4);
		}
	      break;
	     }

/*
 * GET_MAT_2D | matrix
 * (short) ready flag
 * (float) Mat[1,1]
 * ...
 * (float) Mat[3, 2]
 */
	      case GP1_GET_MAT_2D:
	      printf(" | %d, ready flag = %d\n", (uchar) c, GET_SHORT(ptr.sh));
	      goto mat2d;

/*
 * GET_MAT_3D | matrix
 * (short) ready flag
 * (float) Mat[1,1]
 * ...
 * (float) Mat[4,4]
 */
	      case GP1_GET_MAT_3D:
	      printf(" | %d, ready flag = %d\n", (uchar) c, GET_SHORT(ptr.sh));
	      goto mat3d;

/*
 * SET_VPW_2D
 * SET_VPW_3D
 * (float) X scale factor
 * (float) X origin of upper left
 * (float) Y scale factor
 * (float) Y origin of upper left
 */
	      case GP1_SET_VWP_2D:		/* set viewing transform */
	      case GP1_SET_VWP_3D:
	     {
	      float x, y, z, dx, dy, dz;

	      x = GET_FLOAT(ptr.fl); dx = GET_FLOAT(ptr.fl);
	      y = GET_FLOAT(ptr.fl); dy = GET_FLOAT(ptr.fl);
	      z = GET_FLOAT(ptr.fl); dz = GET_FLOAT(ptr.fl);
	      printf("  %f,%f  %f,%f  %f,%f\n", x, dx, y, dy, z, dz);
	      break;
	     }
	     
/*
 * XFVEC_3D
 * (short) number of vectors (each vector is 2 endpoints)
 * (float) X coordinate of endpoint 1 |
 * (float) Y coordinate of endpoint 1 |
 * (float) Z coordinate of endpoint 1 |------- vector 1
 * (float) X coordinate of endpoint 2 |
 * (float) Y coordinate of endpoint 2 |
 * (float) Z coordinate of endpoint 2 |
 *	...
 * (float) X coordinate of endpoint 2n - 1 |
 * (float) Y coordinate of endpoint 2n - 1 |
 * (float) Z coordinate of endpoint 2n - 1 |------- vector n
 * (float) X coordinate of endpoint 2n     |
 * (float) Y coordinate of endpoint 2n     |
 * (float) Z coordinate of endpoint 2n     |
 */
	      case GP1_XFVEC_3D:		/* vector list */
	     {
	      FAST int i;
	      float x1, y1, z1, x2, y2, z2;

	      printf("\t%d vectors\n", n = GET_SHORT(ptr.sh));
	      i = 0;
	      while (--n >= 0)			/* print vector endpoints */
		{
		 x1 = GET_FLOAT(ptr.fl); y1 = GET_FLOAT(ptr.fl); z1 = GET_FLOAT(ptr.fl);
		 x2 = GET_FLOAT(ptr.fl); y2 = GET_FLOAT(ptr.fl); z2 = GET_FLOAT(ptr.fl);
		 if (vpdbg > 1)
		    printf("%d\t%f\t%f\t%f\n\t%f\t%f\t%f\n",
		   	    ++i, x1, y1, z1, x2, y2, z2);
		}
	      break;
	     }

/*
 * XF_LINE_FLT_3D
 * (short) move/draw flag, END FLAG CLEAR
 * (float) X coordinate of vector 1
 * (float) Y coordinate of vector 1
 * (float) Z coordinate of vector 1
 *	...
 * (short) move/draw flag, END FLAG SET
 * (float) X coordinate of endpoint N
 * (float) Y coordinate of endpoint N
 * (float) Z coordinate of endpoint N
 */
	      case GP1_XF_LINE_FLT_3D:
	     {
	      FAST int i, n;
	      float x, y, z;

	      printf("\n");
	      i = 0;
	      do
		{
		 n = GET_SHORT(ptr.sh);
		 x = GET_FLOAT(ptr.fl); y = GET_FLOAT(ptr.fl); z = GET_FLOAT(ptr.fl);
		 if (vpdbg > 1) printf("%d\t%s %f\t%f\t%f\n", ++i,
			(n & GPB_MOVE_FLAG) ? "move" : "draw", x, y, z);
		}
	      while (!(n & GPB_END_FLAG));
	      break;
	     }
/*
 * XF_LINE_FLT_2D
 * (short) move/draw flag, END FLAG CLEAR
 * (float) X coordinate of vector 1
 * (float) Y coordinate of vector 1
 *	...
 * (short) move/draw flag, END FLAG SET
 * (float) X coordinate of endpoint N
 * (float) Y coordinate of endpoint N
 */
	      case GP1_XF_LINE_FLT_2D:
	     {
	      FAST int i, n;
	      float x, y, z;

	      printf("\n");
	      i = 0;
	      do
		{
		 n = GET_SHORT(ptr.sh);
		 x = GET_FLOAT(ptr.fl); y = GET_FLOAT(ptr.fl);
		 if (vpdbg > 1) printf("%d\t%s %f\t%f\n", ++i,
			(n & GPB_MOVE_FLAG) ? "move" : "draw", x, y);
		}
	      while (!(n & GPB_END_FLAG));
	      break;
	     }

/*
 * XF_LINE_INT_3D
 * (short) move/draw flag, END FLAG CLEAR
 * (int) X coordinate of vector 1
 * (int) Y coordinate of vector 1
 * (int) Z coordinate of vector 1
 *	...
 * (short) move/draw flag, END FLAG SET
 * (int) X coordinate of endpoint N
 * (int) Y coordinate of endpoint N
 * (int) Z coordinate of endpoint N
 */
	      case GP1_XF_LINE_INT_3D:
	     {
	      FAST int i, n;
	      int x, y, z;

	      printf("\n");
	      i = 0;
	      do
		{
		 n = GET_SHORT(ptr.sh);
		 x = GET_INT(ptr.in); y = GET_INT(ptr.in); z = GET_INT(ptr.in);
		 if (vpdbg > 1) printf("%d\t%s %d\t%d\t%d\n", ++i,
			(n & GPB_MOVE_FLAG) ? "move" : "draw", x, y, z);
		}
	      while (!(n & GPB_END_FLAG));
	      break;
	     }
/*
 * XF_LINE_INT_2D
 * (short) move/draw flag, END FLAG CLEAR
 * (int) X coordinate of vector 1
 * (int) Y coordinate of vector 1
 *	...
 * (short) move/draw flag, END FLAG SET
 * (int) X coordinate of endpoint N
 * (int) Y coordinate of endpoint N
 */
	      case GP1_XF_LINE_INT_2D:
	     {
	      FAST int i, n;
	      int x, y;

	      printf("\n");
	      i = 0;
	      do
		{
		 n = GET_SHORT(ptr.sh);
		 x = GET_INT(ptr.in); y = GET_INT(ptr.in);
		 if (vpdbg > 1) printf("%d\t%s %d\t%d\n", ++i,
			(n & GPB_MOVE_FLAG) ? "move" : "draw", x, y);
		}
	      while (!(n & GPB_END_FLAG));
	      break;
	     }

/*
 * XFVEC_2D
 * (short) number of vectors (each vector is 2 endpoints)
 * (float) X coordinate of endpoint 1 |
 * (float) Y coordinate of endpoint 1 |------ vector 1
 * (float) X coordinate of endpoint 2 |
 * (float) Y coordinate of endpoint 2 |
 *	...
 * (float) X coordinate of endpoint 2n - 1 |
 * (float) Y coordinate of endpoint 2n - 1 |------- vector n
 * (float) X coordinate of endpoint 2n     |
 * (float) Y coordinate of endpoint 2n     |
 */
	      case GP1_XFVEC_2D:		/* vector list */
	     {
	      FAST int i;
	      float x1, y1, x2, y2;

	      printf("\t%d vectors\n", n = *ptr.sh++);
	      i = 0;
	      while (--n >= 0)			/* print vector endpoints */
		{
		 x1 = GET_INT(ptr.in); y1 = GET_INT(ptr.in);
		 x2 = GET_INT(ptr.in); y2 = GET_INT(ptr.in);
		 if (vpdbg > 1)
		    printf("%d\t%f\t%f\n%f\t%f\n", ++i, x1, y1, x2, y2);
		}
	      break;
	     }

/*
 * CORENDCVEC_3D
 * (short) number of vectors (each vector is 2 endpoints)
 * (int) X coordinate of endpoint 1 |
 * (int) Y coordinate of endpoint 1 |
 * (int) Z coordinate of endpoint 1 |------- vector 1
 * (int) X coordinate of endpoint 2 |
 * (int) Y coordinate of endpoint 2 |
 * (int) Z coordinate of endpoint 2 |
 *	...
 * (int) X coordinate of endpoint 2n - 1 |
 * (int) Y coordinate of endpoint 2n - 1 |
 * (int) Z coordinate of endpoint 2n - 1 |------- vector n
 * (int) X coordinate of endpoint 2n     |
 * (int) Y coordinate of endpoint 2n     |
 * (int) Z coordinate of endpoint 2n     |
 */
	      case GP1_CORENDCVEC_3D:		/* vector list */
	     {
	      FAST int i;
	      int x1, y1, z1, x2, y2, z2;

	      printf("\t%d vectors\n", n = *ptr.sh++);
	      i = 0;
	      while (--n >= 0)			/* print vector endpoints */
		{
		 x1 = GET_INT(ptr.in); y1 = GET_INT(ptr.in); z1 = GET_INT(ptr.in);
		 x2 = GET_INT(ptr.in); y2 = GET_INT(ptr.in); z2 = GET_INT(ptr.in);
		 if (vpdbg > 1)
		    printf("%d\t%d\t%d\t%d\n\t%d\t%d\t%d\n",
			   ++i, x1, y1, z1, x2, y2, z2);
		}
	      break;
	     }

/*
 * CGIVEC
 * (short) number of vectors (each vector is 2 endpoints)
 * (int) X coordinate of endpoint 1 |
 * (int) Y coordinate of endpoint 1 |------ vector 1
 * (int) X coordinate of endpoint 2 |
 * (int) Y coordinate of endpoint 2 |
 *	...
 * (int) X coordinate of endpoint 2n - 1 |
 * (int) Y coordinate of endpoint 2n - 1 |------- vector n
 * (int) X coordinate of endpoint 2n     |
 * (int) Y coordinate of endpoint 2n     |
 */
	      case GP1_CGIVEC:			/* vector list */
	     {
	      FAST int i;
	      int x1, y1, x2, y2;

	      printf("\t%d vectors\n", n = *ptr.sh++);
	      i = 0;
	      while (--n >= 0)			/* print vector endpoints */
		{
		 x1 = GET_INT(ptr.in); y1 = GET_INT(ptr.in);
		 x2 = GET_INT(ptr.in); y2 = GET_INT(ptr.in);
		 if (vpdbg > 1)
		    printf("%d\t%d\t%d\n\t%d\t%d\n", ++i, x1, y2, x2, y2);
		}
	      break;
	     }

/*
 * MUL_POINT_FLT_2D
 * (short) number of points
 * (float) X coordinate of point 1
 * (float) Y coordinate of point 1
 * (short) done flag
 *	...
 * (float) X coordinate of endpoint N
 * (float) Y coordinate of endpoint N
 * (short) done flag
 */
	      case GP1_MUL_POINT_FLT_2D:	/* transform a point list */
	     {
	      FAST int i;
	      float x, y;

	      printf("\t%d points\n", n = *ptr.sh++);
	      i = 0;
	      while (--n >= 0)			/* print points */
		{
		 x = GET_FLOAT(ptr.fl); y = GET_FLOAT(ptr.fl);
		 printf("%d\t%f\t%f\n", ++i, x, y);
		 ++ptr.sh;			/* skip done flag */
		}
	      break;
	     }

/*
 * MUL_POINT_INT_3D
 * (short) number of points
 * (int) X coordinate of point 1
 * (int) Y coordinate of point 1
 * (int) Z coordinate of point 1
 * (int) W coordinate of point 1
 * (short) done flag
 *	...
 * (int) X coordinate of endpoint N
 * (int) Y coordinate of endpoint N
 * (int) Z coordinate of endpoint N
 * (int) W coordinate of endpoint N
 * (short) done flag
 */
	      case GP1_MUL_POINT_INT_3D:	/* transform a point list */
	     {
	      FAST int i;
	      int x, y, z, w;

	      printf("\t%d points\n", n = *ptr.sh++);
	      i = 0;
	      while (--n >= 0)			/* print points */
		{
		 x = GET_INT(ptr.in); y = GET_INT(ptr.in);
		 z = GET_INT(ptr.in); w = GET_INT(ptr.in);
		 printf("%d\t%d\t%d\t%d\t%d\n", ++i, x, y, z, w);
		 ptr.sh++;
		}
	      break;
	     }

/*
 * MUL_POINT_INT_2D
 * (short) number of points
 * (int) X coordinate of point 1
 * (int) Y coordinate of point 1
 * (short) done flag
 *	...
 * (int) X coordinate of endpoint N
 * (int) Y coordinate of endpoint N
 * (short) done flag
 */
	      case GP1_MUL_POINT_INT_2D:	/* transform a point list */
	     {
	      FAST int i;
	      int x, y;

	      printf("\t%d points\n", n = *ptr.sh++);
	      i = 0;
	      while (--n >= 0)			/* print points */
		{
		 x = GET_INT(ptr.in); y = GET_INT(ptr.in);
		 printf("%d\t%d\t%d\n", ++i, x, y);
		 ++ptr.sh;			/* skip done flag */
		}
	      break;
	     }

/*
 * MUL_POINT_FLT_3D
 * (short) number of points
 * (float) X coordinate of point 1
 * (float) Y coordinate of point 1
 * (float) Z coordinate of point 1
 * (float) W coordinate of point 1
 * (short) done flag
 *	...
 * (float) X coordinate of endpoint N
 * (float) Y coordinate of endpoint N
 * (float) Z coordinate of endpoint N
 * (float) W coordinate of endpoint N
 * (short) done flag
 */
	      case GP1_MUL_POINT_FLT_3D:	/* transform a point list */
	     {
	      FAST int i;
	      float x, y, z, w;

	      printf("\t%d points\n", n = *ptr.sh++);
	      i = 0;
	      while (--n >= 0)			/* print points */
		{
		 x = GET_FLOAT(ptr.fl); y = GET_FLOAT(ptr.fl);
		 z = GET_FLOAT(ptr.fl); w = GET_FLOAT(ptr.fl);
		 printf("%f\t%f\t%f\t%f\t%d\n", ++i, x, y, z, w);
		 ptr.sh++;
		}
	      break;
	     }

/*
 * PROC_LINE_FLT_3D
 * (short) number of vectors (each vector is 2 endpoints)
 * (short) clip flag		      |
 * (float) X coordinate of endpoint 1 |
 * (float) Y coordinate of endpoint 1 |
 * (float) Z coordinate of endpoint 1 |------- vector 1
 * (float) X coordinate of endpoint 2 |
 * (float) Y coordinate of endpoint 2 |
 * (float) Z coordinate of endpoint 2 |
 * (short) done flag		      |
 *	...
 * (short) clip flag			   |
 * (float) X coordinate of endpoint 2n - 1 |
 * (float) Y coordinate of endpoint 2n - 1 |
 * (float) Z coordinate of endpoint 2n - 1 |------- vector n
 * (float) X coordinate of endpoint 2n     |
 * (float) Y coordinate of endpoint 2n     |
 * (float) Z coordinate of endpoint 2n     |
 * (short) done flag			   |
 */
	      case GP1_PROC_LINE_FLT_3D:	/* transform a vector list */
	     {
	      FAST int i;
	      float x1, y1, z1, x2, y2, z2;

	      printf("\t%d vectors\n", n = *ptr.sh++);
	      i = 0;
	      while (--n >= 0)			/* print vector endpoints */
		{
		 ++ptr.sh;			/* skip clip flag */
		 x1 = GET_FLOAT(ptr.fl); y1 = GET_FLOAT(ptr.fl); z1 = GET_FLOAT(ptr.fl);
		 x2 = GET_FLOAT(ptr.fl); y2 = GET_FLOAT(ptr.fl); z2 = GET_FLOAT(ptr.fl);
		 printf("%d\t%f\t%f\t%f\n\t%f\t%f\t%f\n",
			++i, x1, y1, z2, x2, y2, z2);
		 ++ptr.sh;			/* skip done flag */
		}
	      break;
	     }

/*
 * PROC_LINE_INT_3D
 * (short) number of vectors (each vector is 2 endpoints)
 * (short) clip flag		    |
 * (int) X coordinate of endpoint 1 |
 * (int) Y coordinate of endpoint 1 |
 * (int) Z coordinate of endpoint 1 |------- vector 1
 * (int) X coordinate of endpoint 2 |
 * (int) Y coordinate of endpoint 2 |
 * (int) Z coordinate of endpoint 2 |
 * (short) done flag		    |
 *	...
 * (short) clip flag			 |
 * (int) X coordinate of endpoint 2n - 1 |
 * (int) Y coordinate of endpoint 2n - 1 |
 * (int) Z coordinate of endpoint 2n - 1 |------- vector n
 * (int) X coordinate of endpoint 2n     |
 * (int) Y coordinate of endpoint 2n     |
 * (int) Z coordinate of endpoint 2n     |
 * (short) done flag			 |
 */
	      case GP1_PROC_LINE_INT_3D:	/* transform a vector list */
	     {
	      FAST int i;
	      int x1, y1, z1, x2, y2, z2;

	      printf("\t%d vectors\n", n = *ptr.sh++);
	      i = 0;
	      while (--n >= 0)			/* print vector endpoints */
		{
		 ++ptr.sh;			/* skip clip flag */
		 x1 = GET_INT(ptr.in); y1 = GET_INT(ptr.in); z1 = GET_INT(ptr.in);
		 x2 = GET_INT(ptr.in); y2 = GET_INT(ptr.in); z2 = GET_INT(ptr.in);
		 printf("%d\t%d\t%d\t%d\n\t%d\t%d\t%d\n",
			++i, x1, y1, z2, x2, y2, z2);
		 ++ptr.sh;			/* skip done flag */
		}
	      break;
	     }

/*
 * PROC_LINE_FLT_2D
 * (short) number of vectors (each vector is 2 endpoints)
 * (short) clip flag		      |
 * (float) X coordinate of endpoint 1 |
 * (float) Y coordinate of endpoint 1 |
 * (float) X coordinate of endpoint 2 |
 * (float) Y coordinate of endpoint 2 |
 * (short) done flag		      |
 *	...
 * (short) clip flag			   |
 * (float) X coordinate of endpoint 2n - 1 |
 * (float) Y coordinate of endpoint 2n - 1 |
 * (float) X coordinate of endpoint 2n     |
 * (float) Y coordinate of endpoint 2n     |
 * (short) done flag			   |
 */
	      case GP1_PROC_LINE_FLT_2D:	/* transform a vector list */
	     {
	      FAST int i;
	      float x1, y1, x2, y2;

	      printf("\t%d vectors\n", n = *ptr.sh++);
	      i = 0;
	      while (--n >= 0)			/* print vector endpoints */
		{
		 ++ptr.sh;			/* skip clip flag */
		 x1 = GET_FLOAT(ptr.fl); y1 = GET_FLOAT(ptr.fl);
		 x2 = GET_FLOAT(ptr.fl); y2 = GET_FLOAT(ptr.fl);
		 printf("%d\t%f\t%f\n\t%f\t%f\n", ++i, x1, y1, x2, y2);
		 ++ptr.sh;			/* skip done flag */
		}
	      break;
	     }

/*
 * PROC_LINE_INT_2D
 * (short) number of vectors (each vector is 2 endpoints)
 * (short) clip flag		    |
 * (int) X coordinate of endpoint 1 |
 * (int) Y coordinate of endpoint 1 |
 * (int) X coordinate of endpoint 2 |
 * (int) Y coordinate of endpoint 2 |
 * (short) done flag		    |
 *	...
 * (short) clip flag			 |
 * (int) X coordinate of endpoint 2n - 1 |
 * (int) Y coordinate of endpoint 2n - 1 |
 * (int) X coordinate of endpoint 2n     |
 * (int) Y coordinate of endpoint 2n     |
 * (short) done flag			 |
 */
	      case GP1_PROC_LINE_INT_2D:	/* transform a vector list */
	     {
	      FAST int i;
	      int x1, y1, x2, y2;

	      printf("\t%d vectors\n", n = *ptr.sh++);
	      i = 0;
	      while (--n >= 0)			/* print vector endpoints */
		{
		 ++ptr.sh;			/* skip clip flag */
		 x1 = GET_INT(ptr.in); y1 = GET_INT(ptr.in);
		 x2 = GET_INT(ptr.in); y2 = GET_INT(ptr.in);
		 printf("%d\t%d\t%d\n\t%d\t%d\n", ++i, x1, y1, x2, y2);
		 ++ptr.sh;			/* skip done flag */
		}
	      break;
	     }

/*
 * XF_PGON_INT_3D | [ SHADE_CONSTANT, SHADE_GOURAUD or SHADE_TEX ]
 * (short) number of polygons
 * (short) number of edges for polygon 1
 * (short) number of edges for polygon 2
 *	...
 * (int) X coordinate of edge 1 |
 * (int) Y coordinate of edge 1 |
 * (int) Z coordinate of edge 1 |
 * (short) color | G shaded only  |
 * (short) 0     |		  |
 *	...			  |---- polygon 1
 * (int) X coordinate of edge n |
 * (int) Y coordinate of edge n |
 * (int) Z coordinate of edge n |
 * (short) color | G shaded only  |
 * (short) 0     |		  |
 *	...
 * (int) X coordinate of edge 1 |
 * (int) Y coordinate of edge 1 |
 * (int) Z coordinate of edge 1 |
 * (short) color | G shaded only  |
 * (short) 0     |		  |
 *	...			  |---- polygon 2
 * (int) X coordinate of edge n |
 * (int) Y coordinate of edge n |
 * (int) Z coordinate of edge n |
 * (short) color | G shaded only  |
 * (short) 0     |		  |
 */
	      case GP1_XF_PGON_INT_3D:		/* 3D polygon */
	     {
	      FAST int i, nv, nb;
	      static string ptype[] = { "CONSTANT", "GOURAUD", "TEX" };

	      nb = GET_SHORT(ptr.sh);
	      printf("\tSHADE_%s, %d bound(s)\n", ptype[c], nb);
	      nv = 0;
	      i = 0;
	      while (--nb >= 0)			/* for each bound */
		{
		 printf("Bound %d, %d edges\n", ++i, n = GET_SHORT(ptr.sh));
		 nv += n;
		}
	      i = 0;
	      while (--nv >= 0)			/* for each vector */
		{
		 int x, y, z;

		 x = GET_INT(ptr.in); y = GET_INT(ptr.in); z = GET_INT(ptr.in);
		 if (vpdbg > 1) printf("%d\t%d\t%d\t%d", ++i, x, y, z);
		 if ((c == GP1_SHADE_GOURAUD) && (vpdbg > 1))
		   {
		    n = GET_SHORT(ptr.sh);
		    printf("\t%d:%d", n, GET_SHORT(ptr.sh));
		   }
		 if (vpdbg > 1) printf("\n");
		}
	      break;
	     }

/*
 * XF_PGON_INT_2D | [ SHADE_CONSTANT or SHADE_TEX ]
 * (short) number of polygons
 * (short) number of edges for polygon 1
 * (short) number of edges for polygon 2
 *	...
 * (int) X coordinate of edge 1 |
 * (int) Y coordinate of edge 1 |
 *	...			  |---- polygon 1
 * (int) X coordinate of edge n |
 * (int) Y coordinate of edge n |
 *	...
 * (int) X coordinate of edge 1 |
 * (int) Y coordinate of edge 1 |
 *	...			  |---- polygon 2
 * (int) X coordinate of edge n |
 * (int) Y coordinate of edge n |
 */
	      case GP1_XF_PGON_INT_2D:		/* 2D polygon */
	     {
	      FAST int i, nv, nb;
	      static string ptype[] = { "CONSTANT", "GOURAUD", "TEX" };

	      nb = GET_SHORT(ptr.sh);
	      printf("\tSHADE_%s, %d bound(s)\n", ptype[c], nb);
	      nv = 0;
	      i = 0;
	      while (--nb >= 0)			/* for each bound */
		{
		 printf("Bound %d, %d edges\n", ++i, n = GET_SHORT(ptr.sh));
		 nv += n;
		}
	      i = 0;
	      while (--nv >= 0)			/* for each vector */
		{
		 int x, y;

		 x = GET_INT(ptr.in); y = GET_INT(ptr.in);
		 if (vpdbg > 1) printf("%d\t%d\t%d\n", ++i, x, y);
		}
	      break;
	     }

/*
 * XF_PGON_FLT_3D | [ SHADE_CONSTANT, SHADE_GOURAUD or SHADE_TEX ]
 * (short) number of polygons
 * (short) number of edges for polygon 1
 * (short) number of edges for polygon 2
 *	...
 * (float) X coordinate of edge 1 |
 * (float) Y coordinate of edge 1 |
 * (float) Z coordinate of edge 1 |
 * (short) color | G shaded only    |
 * (short) 0     |		    |
 *	...			    |---- polygon 1
 * (float) X coordinate of edge n |
 * (float) Y coordinate of edge n |
 * (float) Z coordinate of edge n |
 * (short) color | G shaded only    |
 * (short) 0     |		    |
 *	...
 * (float) X coordinate of edge 1 |
 * (float) Y coordinate of edge 1 |
 * (float) Z coordinate of edge 1 |
 * (short) color | G shaded only    |
 * (short) 0     |		    |
 *	...			    |---- polygon 2
 * (float) X coordinate of edge n |
 * (float) Y coordinate of edge n |
 * (float) Z coordinate of edge n |
 * (short) color | G shaded only    |
 * (short) 0     |		    |
 */
	      case GP1_XF_PGON_FLT_3D:		/* 3D polygon */
	     {
	      FAST int i, nv, nb;
	      static string ptype[] = { "CONSTANT", "GOURAUD", "TEX" };

	      nb = GET_SHORT(ptr.sh);
	      printf("\tSHADE_%s, %d bound(s)\n", ptype[c], nb);
	      nv = 0;
	      i = 0;
	      while (--nb >= 0)			/* for each bound */
		{
		 printf("Bound %d, %d edges\n", ++i, n = GET_SHORT(ptr.sh));
		 nv += n;
		}
	      i = 0;
	      while (--nv >= 0)			/* for each vector */
		{
		 float x, y, z;

		 x = GET_FLOAT(ptr.fl); y = GET_FLOAT(ptr.fl); z = GET_FLOAT(ptr.fl);
		 if (vpdbg > 1) printf("%d\t%f\t%f\t%f", ++i, x, y, z);
		 if ((c == GP1_SHADE_GOURAUD) && (vpdbg > 1))
		   {
		    n = GET_SHORT(ptr.sh);
		    printf("\t%d:%d", n, GET_SHORT(ptr.sh));
		   }
		 if (vpdbg > 1) printf("\n");
		}
	      break;
	     }

/*
 * XF_PGON_FLT_2D
 * (short) number of polygons
 * (short) number of edges for polygon 1
 * (short) number of edges for polygon 2
 *	...
 * (float) X coordinate of edge 1 |
 * (float) Y coordinate of edge 1 |
 *	...			    |---- polygon 1
 * (float) X coordinate of edge n |
 * (float) Y coordinate of edge n |
 *	...
 * (float) X coordinate of edge 1 |
 * (float) Y coordinate of edge 1 |
 *	...			    |---- polygon 2
 * (float) X coordinate of edge n |
 * (float) Y coordinate of edge n |
 */
	      case GP1_XF_PGON_FLT_2D:		/* 2D polygon */
	     {
	      FAST int i, nb, nv;

	      nb = GET_SHORT(ptr.sh);
              printf("\t%s, %d bound(s)\n",
		     (c == GP1_SHADE_TEX) ? "SHADE_TEX" : "", nb);
	      i = 0;
	      while (--nb >= 0)			/* for each bound */
		{
		 printf("Bound %d, %d edges\n", ++i, n = GET_SHORT(ptr.sh));
		 nv += n;
		}
	      i = 0;
	      while (--nv >= 0)			/* for each vector */
		{
		 float x, y;

		 x = GET_FLOAT(ptr.fl); y = GET_FLOAT(ptr.fl);
		 if (vpdbg > 1) printf("%d\t%f\t%f\n", ++i, x, y);
		}
	      break;
	     }

/*
 * SET_CLIP_LIST
 * (short) # of rectangles
 * (short) X position of upper left |
 * (short) Y position of upper left |----- rectangle 1
 * (short) X dimension		    |
 * (short) Y dimension		    |
 * ...
 */
	      case GP1_SET_CLIP_LIST:		/* clip list */
	     {
	      FAST int i;

	      i = 0;
	      n = GET_SHORT(ptr.sh);		/* how many rectangles */
	      printf("\t%d\n", n);
	      while (--n >= 0)
		{
		 int x, y, dx, dy;

		 x = GET_SHORT(ptr.sh); y = GET_SHORT(ptr.sh);
		 dx = GET_SHORT(ptr.sh); dy = GET_SHORT(ptr.sh);
		 printf("%d\t%d,%d  %d,%d\n", ++i, x, y, dx, dy);
	        }
	      break;
	     }
/*
 * SET_ZBUF
 * (short) value to store in Z buffer
 * (short) X position of upper left
 * (short) Y position of upper left
 * (short) X dimension
 * (short) Y dimension
 */
	      case GP1_SET_ZBUF:		/* Z buffer */
	     {
	      int x, y, dx, dy;

	      n = GET_SHORT(ptr.sh);	
	      x = GET_SHORT(ptr.sh); y = GET_SHORT(ptr.sh);
	      dx = GET_SHORT(ptr.sh); dy = GET_SHORT(ptr.sh);
	      printf("\t%d  %d,%d  %d,%d\n", n, x, y, dx, dy);
	     }
	  }
	  }
}

/*
 * Viewport function tables
 */
static	_vpfp	veci2d[] = {	/* vector integer 2D */
	destroy_gp,
	flush_gp,
	print_gp,
	get_gp,
	set_gp,
	draw_vec_i2d,
	box_gp,
	clear_gp,
	move_vec_i2d,
	repaint_gp,
	xform_i2d,
	mrel_vec_i2d,
	oval_gp,
	close_gp,
	drel_vec_i2d,
	wait_gp,
	mapcolor_gp,
};

static	_vpfp	veci3d[] = {	/* vector integer 3D */
	destroy_gp,
	flush_gp,
	print_gp,
	get_gp,
	set_gp,
	draw_vec_i3d,
	box_gp,
	clear_gp,
	move_vec_i3d,
	repaint_gp,
	xform_i3d,
	mrel_vec_i3d,
	oval_gp,
	close_gp,
	drel_vec_i3d,
	wait_gp,
	mapcolor_gp,
};

static	_vpfp	vecf3d[] = {	/* vector floating 3D */
	destroy_gp,
	flush_gp,
	print_gp,
	get_gp,
	set_gp,
	draw_vec_f3d,
	box_gp,
	clear_gp,
	move_vec_f3d,
	repaint_gp,
	xform_f3d,
	mrel_vec_f3d,
	oval_gp,
	close_gp,
	drel_vec_f3d,
	wait_gp,
	mapcolor_gp,
};

static	_vpfp	vecf2d[] = {	/* vector floating 2D */
	destroy_gp,
	flush_gp,
	print_gp,
	get_gp,
	set_gp,
	draw_vec_f2d,
	box_gp,
	clear_gp,
	move_vec_f2d,
	repaint_gp,
	xform_f2d,
	mrel_vec_f2d,
	oval_gp,
	close_gp,
	drel_vec_f2d,
	wait_gp,
	mapcolor_gp,
};

static	_vpfp	polyi2d[] = {	/* polytor integer 2D */
	destroy_gp,
	flush_gp,
	print_gp,
	get_gp,
	set_gp,
	draw_poly_i2d,
	box_gp,
	clear_gp,
	move_poly_i2d,
	repaint_gp,
	xform_i2d,
	mrel_poly_i2d,
	oval_gp,
	close_gp,
	drel_poly_i2d,
	wait_gp,
	mapcolor_gp,
};

static	_vpfp	polyi3d[] = {	/* polytor integer 3D */
	destroy_gp,
	flush_gp,
	print_gp,
	get_gp,
	set_gp,
	draw_poly_i3d,
	box_gp,
	clear_gp,
	move_poly_i3d,
	repaint_gp,
	xform_i3d,
	mrel_poly_i3d,
	oval_gp,
	close_gp,
	drel_poly_i3d,
	wait_gp,
	mapcolor_gp,
};

static	_vpfp	polyf3d[] = {	/* polytor floating 3D */
	destroy_gp,
	flush_gp,
	print_gp,
	get_gp,
	set_gp,
	draw_poly_f3d,
	box_gp,
	clear_gp,
	move_poly_f3d,
	repaint_gp,
	xform_f3d,
	mrel_poly_f3d,
	oval_gp,
	close_gp,
	drel_poly_f3d,
	wait_gp,
	mapcolor_gp,
};

static	_vpfp	polyf2d[] = {	/* polytor floating 2D */
	destroy_gp,
	flush_gp,
	print_gp,
	get_gp,
	set_gp,
	draw_poly_f2d,
	box_gp,
	clear_gp,
	move_poly_f2d,
	repaint_gp,
	xform_f2d,
	mrel_poly_f2d,
	oval_gp,
	close_gp,
	drel_poly_f2d,
	wait_gp,
	mapcolor_gp,
};

static	_vpfp	invisi2d[] = {	/* invisible integer 2D */
	destroy_gp,
	flush_gp,
	print_gp,
	get_gp,
	set_gp,
	move_vec,
	move_vec,
	clear_gp,
	move_vec,
	repaint_gp,
	xform_i2d,
	mrel_vec_i2d,
	mrel_vec_i2d,
	close_gp,
	mrel_vec_i2d,
	wait_gp,
	mapcolor_gp,
};

static	_vpfp	invisi3d[] = {	/* invisible integer 3D */
	destroy_gp,
	flush_gp,
	print_gp,
	get_gp,
	set_gp,
	move_vec,
	move_vec,
	clear_gp,
	move_vec,
	repaint_gp,
	xform_i3d,
	mrel_vec_i3d,
	mrel_vec_i3d,
	close_gp,
	mrel_vec_i3d,
	wait_gp,
	mapcolor_gp,
};

static	_vpfp	invisf2d[] = {	/* invisible floating 2D */
	destroy_gp,
	flush_gp,
	print_gp,
	get_gp,
	set_gp,
	move_vec,
	move_vec,
	clear_gp,
	move_vec,
	repaint_gp,
	xform_f2d,
	mrel_vec_f2d,
	mrel_vec_f2d,
	close_gp,
	mrel_vec_f2d,
	wait_gp,
	mapcolor_gp,
};

static	_vpfp	invisf3d[] = {	/* invisible floating 3D */
	destroy_gp,
	flush_gp,
	print_gp,
	get_gp,
	set_gp,
	move_vec,
	move_vec,
	clear_gp,
	move_vec,
	repaint_gp,
	xform_f3d,
	mrel_vec_f3d,
	mrel_vec_f3d,
	close_gp,
	mrel_vec_f3d,
	wait_gp,
	mapcolor_gp,
};

static	_vpfp	*vpftab[] = {
	veci2d, veci3d, vecf2d, vecf3d,
	polyi2d, polyi3d, polyf2d, polyf3d,
	invisi2d, invisi3d, invisf2d, invisf3d,
	invisi2d, invisi3d, invisf2d, invisf3d,
};

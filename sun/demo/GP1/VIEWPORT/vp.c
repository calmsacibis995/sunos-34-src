#ifndef lint
static	char sccsid[] = "@(#)vp.c 1.3 87/02/23 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/**********
 *
 *
 *
 *	@   @  @@@@
 *	@   @  @   @
 *	@   @  @@@@
 *	 @ @   @
 *	  @    @
 *
 *	Create_VP(t, c, pw)		create viewport
 *
 *	vp_dbuf_create(vp)		initialize double buffering area
 *	vp_dbuf_set(vp, attr, val)	set double buffering attribute
 *
 *	vp_pick_set(vp, attr, val)	set picking attribute
 *	vp_pick_get(vp, attr)		get picking attribute
 *
 *	Set_Texture(vpobj, n, type, texture)	assign line texture
 *	Set_Pattern(vpobj, n, bitmap)		assign polygon pattern
 *
 *
 **********/
#include "vp_int.h"
#include "bitmap.h"
#include <pixrect/pixrect_hs.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>

/***
 *
 * DBUF - double buffering information
 *
 ***/
typedef struct
  {
   uchar	db_mask1;		/* frame 1 pixel planes mask */
   uchar	db_mask2;		/* frame 2 pixel planes mask */
   uchar	db_flags;		/* info flags */
   uchar	db_red1[256];		/* frame 1 red color map */
   uchar	db_green1[256];		/* frame 1 green color map */
   uchar	db_blue1[256];		/* frame 1 blue color map */
   uchar	db_red2[256];		/* frame 2 red color map */
   uchar	db_green2[256];		/* frame 2 green color map */
   uchar	db_blue2[256];		/* frame 2 blue color map */
  } DBUF;

/***
 *
 * PICK - pick information
 *
 ***/
typedef struct
  {
   struct pixrect *pk_pr1;	/* pick area pixrect */
   struct pixrect *pk_pr2;	/* another pick area pixrect */
   short	pk_depth;	/* pick depth */
   short	pk_x;		/* X origin of pick area */
   short	pk_y;		/* Y origin of pick area */
   short	pk_dx;		/* X size of pick area */
   short	pk_dy;		/* Y size of pick area */
  } PICK;

/****
 *
 * cindex1-4 - color index tables
 * The colors 0 - N (N bits per pixel) are mapped into both image buffers
 * cindex selects which color index table to use based on bits per pixel.
 *
 * lowbufmask - image buffer masks for low-order pixel image buffers
 * hibufmask - image buffer masks for hi-order pixel image buffers
 *
 * cmapsize - translates bits per pixel into color map size
 *
 * cmapname - names of color maps based on bits per pixel
 *
 ****/
static	uchar	cindex0[256] = {
0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEE,0xED,0xEE,0xEF,
0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFF,0xFD,0xFE,0xFF,
};

static	uchar	cindex1[] = { 0, 3 };
static	uchar	cindex2[] = { 0, 5, 0XA, 0XF };
static	uchar	cindex3[] = { 0, 011, 022, 033, 044, 055, 066, 077 };
static	uchar	cindex4[] = { 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
			      0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
static	uchar	*cindex[] = { cindex1, cindex1, cindex2, cindex3, cindex4,
				cindex0, cindex0, cindex0, cindex0 };

static	uchar	lowbufmask[] = { 0, 1, 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 0xff };
static	uchar	hibufmask[] = { 0, 2, 0xC, 070, 0xf0, 0, 0, 0, 0 };

static	uchar	cmapsize[] = { 0, 2, 4, 8, 16, 32, 64, 128, 256 };
static	string	cmapname[] = { NULL, "dbuf1", "dbuf2", "dbuf3", "dbuf4",
				     "dbuf5", "dbuf6", "dbuf7", "dbuf8" };

/****
 *
 * vp_putcolormap(vp)
 * Wait for vertical retrace (device specific)
 *
 ****/
static
vp_putcolormap(vp, n, r, g, b)
/*    --needs--			 */
	VPDATA	*vp;
	int	n;
	uchar	*r, *g, *b;
{
/*    --uses--			 */
extern	int	gp1_rop();
extern	int	cg2_rop();
FAST	struct pixrect *pr;

	pr = vp->vp_win->pw_pixrect;
	if (pr->pr_ops->pro_rop == gp1_rop)
	  {
	   while ((gp1_d(pr)->cgpr_va)->status.word & 0x80 == 1);
	   while ((gp1_d(pr)->cgpr_va)->status.word & 0x80 == 0);
	   while ((gp1_d(pr)->cgpr_va)->status.word & 0x80 == 1);
	  }
	else if (pr->pr_ops->pro_rop == cg2_rop)
	  {
	   while ((cg2_d(pr)->cgpr_va)->status.word & 0x80 == 1);
	   while ((cg2_d(pr)->cgpr_va)->status.word & 0x80 == 0);
	   while ((cg2_d(pr)->cgpr_va)->status.word & 0x80 == 1);
	  }
	pw_putcolormap(vp->vp_win, 0, n, r, g, b);
}

/****
 *
 * vp_dbuf_create(vpobj)
 * Allocate double buffering data area. This area consists of a header
 * with size information and expanded RGB color maps for both frames.
 * Color maps used in double buffering may contain at most 8 entries.
 *
 ****/
vp_dbuf_create(vpobj)
/*    --needs--			 */
	VP	vpobj;		/* viewport object */
{
/*    --uses--			 */
local	uchar	*cindex[];	/* color index tables */
local	uchar	lowbufmask[];	/* low order frame buffer pixel masks */
local	uchar	hibufmask[];	/* high order frame buffer pixel masks */
local	uchar	cmapsize[];	/* color map sizes */
local	string	cmapname[];	/* color map names */

FAST	VPDATA	*vp;
FAST	DBUF	*dbuf;
FAST	int	nbits, n;	/* # of bits per pixel being used */

	vp = vp_addr(vpobj);
	nbits = vp->vp_pixbits;			/* # bits per pixel */
	if (nbits <= 0)				/* turn off pixbits */
	  {
	   if (!IsNull(vp->vp_dbuf)) FreeObj(vp->vp_dbuf);
	   return (0);
	  }
	if ((dbuf = ObjAddr(vp->vp_dbuf, DBUF)) == NULL)
	  {
	   if (IsNull(vp->vp_dbuf = NewObj(T_IOBJ, sizeof(DBUF))))
	      error("init_dbuf: can't make double buffering data area\n", 0, 0);
	   dbuf = ObjAddr(vp->vp_dbuf, DBUF);
	   vp->vp_cmap_red = dbuf->db_red1;	/* initialize color ... */
	   vp->vp_cmap_green = dbuf->db_green1;	/* map pointers */
	   vp->vp_cmap_blue = dbuf->db_blue1;
	   vp->vp_cmap_red[1] = 255;		/* initialize color 1 */
	   vp->vp_cmap_green[1] = 255;
	   vp->vp_cmap_blue[1] = 255;
	  }
	if (nbits > 8) nbits = 8;		/* 8 is maximum */
	vp->vp_cmap_size = cmapsize[nbits];	/* color map size */
	dbuf->db_mask1 = lowbufmask[nbits];	/* pixel planes masks */
	dbuf->db_mask2 = hibufmask[nbits];	/* for both frames */
	vp->vp_cindex = cindex[nbits];		/* color map index and name */
	pw_setcmsname(vp->vp_win, cmapname[nbits]);
	vp->vp_pixplanes = dbuf->db_mask1;	/* frame mask */
	vp->vp_frame = 0;
	dbuf->db_flags = 1;			/* reload color maps */
	return (1);
}


/****
 *
 * vp_dbuf_set(vpobj, attr, val)
 * Set double buffering attribute
 *
 * VP_FRAME	frame buffer being used (0 or 1)
 * VP_PIXPLANES	set pixel planes mask
 * VP_DBUF	double buffering status
 *		VP_DBUF_ENABLE	enable double buffering
 *		VP_DBUF_DISABLE	disable, single frame only
 *		VP_DBUF_REMAP	reload color maps
 *
 ****/
vp_dbuf_set(vpobj, attr, val)
/*    --needs--			 */
	VP	vpobj;		/* viewport object */
	int	attr, val;
{
/*    --calls--			 */
local	void	dbuf_cmap();	/* load color maps */

/*    --uses--			 */
FAST	VPDATA	*vp;		/* -> viewport */
FAST	DBUF	*dbuf;
	int	mask;

	vp = vp_addr(vpobj);
	if ((dbuf = ObjAddr(vp->vp_dbuf, DBUF)) == NULL) return;
	switch (attr)
	  {
/*
 * dbuf_set(vp, VP_FRAME, 0) use frame buffer 0 (low order color map)
 * dbuf_set(vp, VP_FRAME, 1) use frame buffer 1 (hi order color map)
 */
	   case VP_FRAME:
	   Flush_VP(vpobj);			/* end current frame */
	   if (vp->vp_dbuf_stat != VP_DBUF_ENABLE) return;
frame:	   if (dbuf->db_flags) dbuf_cmap(vp);
	   switch (val)
	     {
	      case 0:
	      mask = dbuf->db_mask2 & (vp->vp_pixplanes << vp->vp_pixbits);
	      vp_putcolormap(vp, vp->vp_cmap_size * vp->vp_cmap_size,
			dbuf->db_red1, dbuf->db_green1, dbuf->db_blue1);
	      break;
	
	      case 1:
	      mask = dbuf->db_mask1 & vp->vp_pixplanes;
	      vp_putcolormap(vp, vp->vp_cmap_size * vp->vp_cmap_size,
			dbuf->db_red2, dbuf->db_green2, dbuf->db_blue2);
	      break;
	     }
	   Set_VP(vpobj, VP_REALPLANES, mask);
	   break;

/*
 * dbuf_set(vp, VP_DBUF, VP_DBUF_DISABLE)
 *	turn double buffering off, use frame 1 color map
 * dbuf_set(vp, VP_DBUF, VP_DBUF_ENABLE)
 *	double buffering on, use frame 1 color map and mask
 * dbuf_set(vp, VP_DBUF, VP_DBUF_REMAP)
 *	reload color maps
 */
	   case VP_DBUF_STAT:
	   if (val == VP_DBUF_REMAP)		/* remap colors? */
	      val = vp->vp_dbuf_stat;
	   Flush_VP(vpobj);			/* end current frame */
	   Set_VP(vpobj, VP_COLOR, vp->vp_color);
	   vp->vp_dbuf_stat = val;
	   switch (val)
	     {
	      case VP_DBUF_DISABLE:		/* not double buffered */
	      vp->vp_frame = 0;
	      mask = dbuf->db_mask1 & vp->vp_pixplanes;
	      vp_putcolormap(vp, vp->vp_cmap_size,
			dbuf->db_red1, dbuf->db_green1, dbuf->db_blue1);
	      Set_VP(vpobj, VP_REALPLANES, mask);
	      pw_putattributes(vp->vp_win, &mask);
	      break;

	      case VP_DBUF_ENABLE:		/* use low order table */
	      if (vp->vp_pixbits > 4) return;	/* ignore double buffering */
	      dbuf_cmap(vp);
	      val = vp->vp_frame;
	      goto frame;
#if	0
	      vp_putcolormap(vp, vp->vp_cmap_size * vp->vp_cmap_size,
			dbuf->db_red1, dbuf->db_green1, dbuf->db_blue1);
	      if (vp->vp_dbuf_stat == VP_DBUF_ENABLE) break;
	      vp->vp_frame = 0;
	      mask = dbuf->db_mask2 & (vp->vp_pixplanes << vp->vp_pixbits);
	      Set_VP(vpobj, VP_REALPLANES, mask);
	      mask = dbuf->db_mask1 | dbuf->db_mask2;
	      pw_putattributes(vp->vp_win, &mask);
	      break;
#endif
	     }
	   break;

/*
 * dbuf_set(vp, VP_PIXPLANES, n) set pixel planes mask
 */
	   case VP_PIXPLANES:
	   if ((vp->vp_dbuf_stat == VP_DBUF_ENABLE) && (vp->vp_frame == 0))
	      mask = dbuf->db_mask2 & (val << vp->vp_pixbits);
	   else mask = dbuf->db_mask1 & val;
	   Set_VP(vpobj, VP_REALPLANES, mask);
	  }
}

/****
 *
 * dbuf_cmap(vp)
 * Loads the color maps for double buffering.
 * 1. propagate user color map into all rows of frame 1 maps
 *    and along all columns of frame 2 maps
 * 2. set pixel plane mask
 * 3. download the constructed color maps
 *
 ****/
static void
dbuf_cmap(vp)
/*    --needs--			 */
FAST	VPDATA	*vp;
{
/*    --uses--			 */
FAST	DBUF	*dbuf;
FAST	int	n, i, j;

	dbuf = ObjAddr(vp->vp_dbuf, DBUF);
	dbuf->db_flags = 0;
	n = vp->vp_cmap_size;
	for (i = 0; i < n; ++i)
	   for (j = 0; j < n; ++j)
	     {
	      dbuf->db_red1[i * n + j] = dbuf->db_red1[j];
	      dbuf->db_green1[i * n + j] = dbuf->db_green1[j];
	      dbuf->db_blue1[i * n + j] = dbuf->db_blue1[j];
	      dbuf->db_red2[j * n + i] = dbuf->db_red1[j];
	      dbuf->db_green2[j * n + i] = dbuf->db_green1[j];
	      dbuf->db_blue2[j * n + i] = dbuf->db_blue1[j];
	     }
}

/****
 *
 * vp_pick_set(vpobj, attr, val)
 * Set pick attribute
 *
 * VP_PICK_ID	Pick ID. Read the pick screen area into a pixrect.
 *		Setting to zero turns picking off and removes pick info.
 *		returns the previous pick ID if there was a pick, else 0
 * VP_PICK_ORG	Pick area origin. Center of pick area rectangle.
 * VP_PICK_DIM	Pick area dimensions. Make new pick area pixrect.
 *
 ****/
vp_pick_set(vpobj, attr, val)
/*    --needs--			 */
	VP	vpobj;		/* viewport object */
	int	attr, val;
{
/*    --uses--			 */
FAST	VPDATA	*vp;
FAST	PICK	*pick;

/*
 * pk_set(vp, VP_PICK_ID, 0)
 * turn picking off, garbage collect pick info
 */
	vp = vp_addr(vpobj);
	if ((attr == VP_PICK_ID) && (val == 0))
	  {
	   if ((pick = ObjAddr(vp->vp_pick, PICK)) == NULL)
	      return;
	   vp->vp_pick_id = 0;			/* turn picking off for now */
	   vp->vp_pick_count = 0;
	   return;
	  }
/*
 * The other attributes need a pick area to be created.
 */
	if ((pick = ObjAddr(vp->vp_pick, PICK)) == NULL)
	  {
	   if (IsNull(vp->vp_pick = NewObj(T_IOBJ, sizeof(PICK))))
	      error("vp_pick_set: can't make pick data areas\n", 0, 0);
	   pick = ObjAddr(vp->vp_pick, PICK);
	   pick->pk_depth = 8;			/* assume depth of 8 */
	  }
	switch (attr)
	  {
/*
 * pk_set(vp, VP_PICK_ID, n)
 * if the pick ID is already non-zero, we do a "get" on the old VP_PICK_ID
 * to determine if there was a pick. If so, we return this pick status,
 * set the new pick ID and continue. It is assumed that setup of the pixrect
 * areas, etc. has already been done since the pick ID was nonzero.
 *
 * If the pick ID is zero, picking was previously off.
 * If there is no pixrect yet, but dimensions have been specified
 * one is created. The screen area specified by the pick origin and
 * dimensions is read into the memory pixrect.
 */
	   case VP_PICK_ID:
	   vp->vp_pick_count = 0;
	   if (vp->vp_pick_id)			/* already non-zero */
	     {
	      attr = vp_pick_get(vp, VP_PICK_ID);
	      vp->vp_pick_id = val;		/* reset pick ID */
	      return (attr);			/* return pick status */
	     }
	   vp->vp_pick_id = val;		/* new pick ID value */
save:	   if (pick->pk_pr1 == NULL)		/* no pixrect? */
	     {
	      if ((pick->pk_dx <= 0) ||		/* no size yet? */
		  (pick ->pk_dy <= 0))
		{
		 vp->vp_pick_id = 0;		/* turn picking off */
	         return (0);
		}
	      if ((pick->pk_pr1 =		/* create pixrect area */
		 mem_create(pick->pk_dx, pick->pk_dy,
			pick->pk_depth)) == NULL)
		 error("vp_pick_set: can't make pixrect (mem_create failure)\n", 0, 0);
	     }
	   WAIT_VP(vpobj);			/* flush previous graphics */
	   pw_read(pick->pk_pr1, 0, 0,		/* save screen area */
		   pick->pk_dx, pick->pk_dy, PIX_SRC, vp->vp_win,
		   pick->pk_x - pick->pk_dx / 2, pick->pk_y - pick->pk_dy / 2);
	   return (0);
	       
/*
 * pk_set(vp, VP_PICK_ORG, p)
 * Set pick area origin to specified POINT. The point is assumed to be
 * in 2D integer format.
 */
	   case VP_PICK_ORG:			/* pick area origin */
	   vp->vp_pick_org = *((POINT *) val);
	   pick->pk_x = ((POINT *) val)->i.x;
	   pick->pk_y = ((POINT *) val)->i.y;
	   if (vp->vp_pick_id) goto save;	/* no pending pick */
	   break;

/*
 * pk_set(vp, VP_PICK_DIM, p)
 * Set pick area dimensions to specified POINT. The point is assumed to be
 * in 2D integer format. If a memory pixrect exists (old dimensions),
 * remove it. Allocate a new memory pixrect with the new dimensions.
 */
	   case VP_PICK_DIM:			/* pick area size */
	   vp->vp_pick_dim = *((POINT *) val);
	   pick->pk_dx = ((POINT *) val)->i.x;
	   pick->pk_dy = ((POINT *) val)->i.y;
	   if (pick->pk_pr1)			/* have pixrect? */
	     {
	      pr_destroy(pick->pk_pr1);		/* destroy it */
	      pick->pk_pr1 = NULL;		/* mark it gone */
	     }
	   if (pick->pk_pr2)
	     {
	      pr_destroy(pick->pk_pr2);
	      pick->pk_pr2 = NULL;		/* mark it gone */
	     }
	   if (vp->vp_pick_id) goto save;	/* no pending pick */
	   break;
	  }
	return (0);
}

/****
 *
 * vp_pick_get(vpobj, attr, val)
 * Get pick attribute VP_PICK_ID (Pick ID). Read the pick screen area and
 * compare it with the saved area (saved by setting VP_PICK_ID). If it is
 * different, indicate a pick. The pick ID is not changed and the new
 * screen area is remembered so that another "get" of the pick ID can be
 * performed (without a "set" in between).
 *
 * returns:
 *   previous value of VP_PICK_ID if there was a pick
 *   else 0 (and VP_PICK_ID is zeroed)
 *   VP_PICK_COUNT is set to 1 to indicate a pick (0 for no pick)
 *
 * notes:
 * The contents of the screen is compared with the saved pick area.
 * If the screen has not changed, the memory images will be the same.
 *
 ****/
vp_pick_get(vpobj, attr)
/*    --needs--			 */
	VP	vpobj;		/* viewport object */
	int	attr;
{
/*    --uses--			 */
FAST	VPDATA	*vp;
FAST	PICK	*pick;		/* -> pick info */
FAST	short	*ptr1, *ptr2;	/* -> memory pixrect data */
FAST	int	count, n;	/* # words in memory pixrect */
	struct pixrect *temp;

	vp = vp_addr(vpobj);
	if ((attr != VP_PICK_ID) ||		/* not PICK_ID? */
	    ((n = vp->vp_pick_id) == 0) ||	/* not picking? */
	    ((pick = ObjAddr(vp->vp_pick, PICK)) == NULL) ||
	    (pick->pk_pr1 == NULL))		/* no pick area? */
	   return (0);				/* return NO PICK */
	vp->vp_pick_count = 0;
	if ((pick->pk_pr2 == NULL) &&		/* no compare area? */
	    (pick->pk_pr2 =			/* make pixrect compare area */
	     mem_create(pick->pk_dx, pick->pk_dy, pick->pk_depth)) == NULL)
	   error("vp_pick_set: can't make pixrect (mem_create failure)\n",
		0, 0);
	temp = pick->pk_pr1;			/* swap the two */
	pick->pk_pr1 = pick->pk_pr2;
	pick->pk_pr2 = temp;
	WAIT_VP(vpobj);				/* flush and wait */
	pw_read(pick->pk_pr1, 0, 0,
		pick->pk_dx, pick->pk_dy, PIX_SRC, vp->vp_win,
		pick->pk_x - pick->pk_dx / 2, pick->pk_y - pick->pk_dy / 2);
	count = mpr_mdlinebytes(temp) * pick->pk_dy / sizeof(short);
	ptr1 = mpr_d(pick->pk_pr1)->md_image;	/* -> new pixrect image */
	ptr2 = mpr_d(pick->pk_pr2)->md_image;	/* -> old pixrect image */
	while (--count > 0)			/* check next word */
	   if (*ptr1++ != *ptr2++)
	     {
	      vp->vp_pick_count = 1;		/* indicate a pick */
	      return (n);			/* was a pick */
	     }
	return (0);
}

/****
 *
 * Set_Pattern(vpobj, n, bmobj)
 * Set pattern number N to the given BITMAP. BITMAP objects are created by
 * Create_Bitmap. They may be derived from pixrects or from explicit
 * pixel data.
 *
 * returns:
 *   YES = pattern successfully set
 *   NO = pattern could not be set
 *
 * notes:
 * Each pattern is a BITMAP object that contains the data for the pattern.
 * These pattern objects are kept in an object table (vp_pattab) in the
 * viewport.
 *
 ****/
Set_Pattern(vpobj, n, bmobj)
/*    --needs--			 */
	VP	vpobj;
	int	n;		/* texture number to set */
	BITMAP	bmobj;		/* bitmap object */
{
/*    --uses--			 */
FAST	VPDATA	*vp;

	vp = ObjAddr(vpobj, VPDATA);
	if ((n < 1) || (n > VP_PAT_MAX))	/* offset too large */
	   error("Set_Pattern: pattern #%d out of range\n", n, NO);
	if (IsNull(vp->vp_pattab) &&		/* no pattern table? */
	    IsNull(vp->vp_pattab =		/* allocate pattern table */
		NewObj(T_OOBJ, VP_PAT_MAX * sizeof(OBJID))))
	   error("Set_Pattern: cannot create pattern #%d\n", n, NO);
	PutObjVal(vp->vp_pattab, (n - 1) * sizeof(OBJID), bmobj);
	return (YES);
}

/****
 *
 * Set_Texture(vpobj, n, type, texture)
 * Set texture number N to the given TEXTURE. The TYPE of the texture may be:
 *   VP_TEX_5080	TEXTURE is a bit mask telling which pixels are on
 *			and off along the vector (5080 compatible)
 *   VP_TEX_RANGE	TEXTURE is an array of ranges, telling how many
 *			pixels are on and off along the vector
 *
 * returns:
 *   YES = texture successfully created and set
 *   NO = texture could not be set
 *
 * notes:
 * Each texture is an object that contains the pattern data for the texture.
 * These texture objects are kept in an object table (vp_textab) in the
 * viewport.
 *
 ****/
Set_Texture(vpobj, n, type, txt)
/*    --needs--			 */
	VP	vpobj;
	int	n;		/* texture number to set */
	int	type;		/* type of texture */
	short	*txt;		/* texture data */
{
/*    --uses--			 */
FAST	VPDATA	*vp;
FAST	short	*tptr;
FAST	OBJID	patobj;		/* pattern object */
FAST	int	s;

	vp = ObjAddr(vpobj, VPDATA);
	if ((n < 1) || (n > VP_TEX_MAX))	/* offset too large */
	   error("Set_Texture: texture #%d out of range\n", n, 0);
	if (IsNull(vp->vp_textab) &&		/* no texture table? */
	    IsNull(vp->vp_textab =		/* allocate texture table */
		NewObj(T_OOBJ, VP_TEX_MAX * sizeof(OBJID))))
	   error("Set_Texture: cannot create texture #%d\n", n, NO);
	s = sizeof(short); tptr = txt;
	while (*tptr++) s += sizeof(short);	/* determine texture size */
	if (IsNull(patobj =			/* create pattern object */
		NewObj(T_IOBJ, s + sizeof(short))))
	   error("Set_Texture: cannot create texture #%d\n", n, NO);
	tptr = ObjAddr(patobj, short);
	*tptr++ = type;
	bcopy(txt, tptr, s);
	PutObjVal(vp->vp_textab, (n - 1) * sizeof(OBJID), patobj);
	return (YES);
}

/*****
 *
 * vp_tex_range(maskdata)
 * Converts a 5080 bit mask texture to a texture range.
 * The 5080 texture data is 4 integers. Each bit set specifies the
 * pen should be down for the next 4 pixels. Each clear bit specifies
 * pen up (no draw). The viewport texture data is a null-terminated
 * array of shorts. Each entry gives the length (in pixels) of the
 * next segment along the vector. The first entry is pen down, the
 * second entry is pen up, third is pen down, etc.
 *
 * returns:
 *   object ID of the list containing the resulting texture data
 *
 * notes:
 * To set a texture based on the returned list, use the following:
 *	texobj = make_tex_range(maskdata);
 *	Set_Texture(vp, VP_TEX_RANGE, LstAddr(texobj, short));
 *	FreeObj(texobj);
 *
 ****/
#define	PENDOWN	0x80000000

OBJID
vp_tex_range(maskdata)
/*    --needs--                  */
	int     maskdata[4];	/* 32 byte 5080 texture mask */
{
/*    --uses--                   */
FAST    int     *maskptr;	/* -> next mask word */
FAST    OBJID   texobj;		/* texture range object */
FAST    int     i;		/* bit counter */
FAST    int     mask;		/* current mask word */
FAST    short   length;		/* current segment length */
FAST    int     penbit;		/* pen status bit */

	maskptr = &maskdata[-1];		/* -> mask area */
	penbit = PENDOWN & maskdata[0];		/* pen status of 1st bit */
	length = 0;
	texobj = NewLst(T_ILST, 4 * sizeof(short));
	while (++maskptr < &maskdata[4])	/* for entire 5080 mask */
	  {
	   mask = *maskptr;			/* get next mask word */
	   for (i = 1; i <= 32; ++i)		/* do 32 bits of the entry */
	     {
	      if (penbit ^ (mask & PENDOWN))	/* pen status changed? */
	        {
	         penbit ^= PENDOWN;		/* flip state of pen */
	         texobj = AppWrdLst(texobj, length);
	         length = 4;			/* add 4 pixels to new seg */
	        }
	      else length += 4;			/* add 4 pixels to this seg */
	      mask <<= 1;			/* get next pixel status */
	     }
	  }   
	texobj = AppWrdLst(texobj, length);	/* flush remainder */
	texobj = AppWrdLst(texobj, 0);		/* add ending zero segment */
	return (texobj);
}

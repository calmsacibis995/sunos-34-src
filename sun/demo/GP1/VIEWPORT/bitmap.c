#ifndef lint
static	char sccsid[] = "@(#)bitmap.c 1.3 87/02/23 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/**********
 *
 *
 *	@@@@    @@@   @@@@@  @   @   @@@   @@@@
 *	@   @    @      @    @@ @@  @   @  @   @
 *	@@@@     @      @    @ @ @  @@@@@  @@@@
 *	@   @    @      @    @   @  @   @  @
 *	@@@@    @@@     @    @   @  @   @  @
 *
 *	BITMAP - Bitmap manipulation routines
 *	Create_Bitmap(type, data, w, h)	create bitmap
 *	Destroy_Bitmap(bmobj)		destroy bitmap
 *	Print_Bitmap(bmobj)		print bitmap
 *	Set_Bitmap(bmobj, n, val) 	set bitmap attribute
 *	Get_Bitmap(bmobj, n)	 	get bitmap attribute
 *
 *
 *
 **********/
#include "bitmap_int.h"

/****
 *
 * Create_Bitmap(type, data, width, height)
 * Create bitmap object.
 *
 * returns:
 *   Object ID of bitmap
 *   NULL if no bitmap can be created
 *
 ****/
BITMAP
Create_Bitmap(type, data, width, height, depth)
/*    --needs--			 */
	int	type;		/* bitmap type (BMAP_TYPE_PACKED/UNPACKED) */
FAST	uchar	*data;		/* -> bitmap data */
	int	width, height;	/* width and height in pixels */
	int	depth;		/* bits per pixel */
{
/*    --calls--			 */
extern	struct pixrect *bmap_make_pixrect();

/*    --uses--			 */
FAST	BMDATA	*bm;		/* -> bitmap data */
	BITMAP	bmobj;		/* bitmap object */

	if (IsNull(bmobj = NewObj(T_IOBJ, sizeof(BMDATA))))
	   error("Create_Bitmap: cannot make bitmap\n", 0, 0);
	bm = ObjAddr(bmobj, BMDATA);
	bm->bm_type = type;
	bm->bm_width = width;
	bm->bm_height = height;
	bm->bm_depth = depth;
	if (bmap_make_pixrect(bm, data)) return (bmobj);
	FreeObj(bmobj);
	return (0);
}

/****
 *
 * Destroy_Bitmap(bmobj)
 * Destroy bitmap and all storage associated with it.
 * If the type is BMAP_TYPE_PIXRECT, the pixrect is not freed.
 *
 ****/
Destroy_Bitmap(bmobj)
/*    --needs--			 */
	OBJID	bmobj;		/* bitmap to destroy */
{
/*    --uses--			 */
FAST	BMDATA	*bm;		/* -> bitmap data */

	bm = ObjAddr(bmobj, BMDATA);
	if (bm->bm_type != BMAP_TYPE_PIXRECT)	/* destroy the pixrect */
	   free(bm->bm_pixrect);		/* if necessary */
	FreeObj(bmobj);
}

/****
 *
 * Print_Bitmap(bmobj)
 * Print bitmap header info (not data)
 *
 ****/
Print_Bitmap(bmobj)
/*    --needs--			 */
	OBJID	bmobj;		/* bitmap to destroy */
{
/*    --uses--			 */
FAST	BMDATA	*bm;		/* -> bitmap data */
	string	type;

	bm = ObjAddr(bmobj, BMDATA);
	switch (bm->bm_type)
	  {
	   case BMAP_TYPE_PIXRECT: type = "PIXRECT"; break;
	   case BMAP_TYPE_PACKED: type = "PACKED"; break;
	   case BMAP_TYPE_UNPACKED: type = "UNPACKED"; break;
	  }
	printf("type = %s, width = %d, height = %d, depth = %d\n",
		type, bm->bm_width, bm->bm_height, bm->bm_depth);
}

/****
 *
 * Get_Bitmap(bmobj, attr)
 * Get bitmap attribute.
 *
 * Set_Bitmap(bmobj, attr, val)
 * Set bitmap attribute.
 *   BMAP_WIDTH		width of bitmap in pixels
 *   BMAP_HEIGHT	height of bitmap in pixels
 *   BMAP_DEPTH		depth of bitmap (1, 2, 4, 8)
 *   BMAP_DATA		data area for bitmap (write only)
 *   BMAP_PIXRECT	-> pixrect for bitmap
 *
 ****/
Get_Bitmap(bmobj, attr)
/*    --needs--			 */
	BITMAP	bmobj;
	int	attr;
{
/*    --uses--			 */
FAST	BMDATA	*bm;		/* -> bitmap info */

	bm = ObjAddr(bmobj, BMDATA);
	switch (attr)
       {
	case BMAP_WIDTH: return (bm->bm_width);
	case BMAP_HEIGHT: return (bm->bm_height);
	case BMAP_DEPTH: return (bm->bm_depth);
	case BMAP_PIXRECT: return ((int) bm->bm_pixrect);
	case BMAP_SIZE:
	return ((bm->bm_depth == 1) ?
		(bm->bm_height * (bm->bm_width + 15) / 16) :
		((bm->bm_height * bm->bm_width) + 1) & ~1);
	default: return (0);
       }
}

Set_Bitmap(bmobj, attr, val)
/*    --needs--			 */
	BITMAP	bmobj;
	int	attr, val;
{
/*    --calls--			 */
local	struct pixrect *bmap_make_pixrect();

/*    --uses--			 */
FAST	BMDATA	*bm;		/* -> bitmap info */

	bm = ObjAddr(bmobj, BMDATA);
	switch (attr)
       {
	case BMAP_WIDTH: bm->bm_width = val; break;
	case BMAP_HEIGHT: bm->bm_height = val; break;
	case BMAP_DEPTH: bm->bm_depth = val; break;
	case BMAP_DATA: bmap_make_pixrect(bm, val); break;
	case BMAP_PIXRECT: bm->bm_pixrect = (struct pixrect *) val; break;
       }
	return (0);
}

/****
 *
 * bmap_make_pixrect(bm, data)
 * Make the pixrect for a bitmap based on its current width/height
 * and data area.
 *
 * returns:
 *   -> pixrect for bitmap, NULL if it cannot be created
 *
 * notes:
 * The bm_pixrect field of the bitmap structure is assigned the
 * pixrect pointer. If the type is PIXRECT, the DATA argument is
 * assumed to be a pixrect pointer and is used as the new pixrect.
 * In this case, the old pixrect is not freed. Other types garbage
 * collect the old pixrect before creating and assigning a new one.
 *
 ****/
static struct pixrect *
bmap_make_pixrect(bm, data)
/*    --needs--			 */
FAST	BMDATA	*bm;		/* -> bitmap structure */
FAST	uchar	*data;		/* -> data area */
{
	if ((bm->bm_width <= 0) || (bm->bm_height <= 0))
	   error("Create_Bitmap: illegal width or height for bitmap\n", 0, 0);
/*
 * If the data is already a pixrect, just assign it and return.
 * If not, create a memory pixrect of the appropriate size and
 * store the data in it. Free any pixrect that may already be
 * associated with the bitmap.
 */
	if (bm->bm_type == BMAP_TYPE_PIXRECT)	/* data is a pixrect */
	   return (bm->bm_pixrect = (struct pixrect *) data);
	if (bm->bm_pixrect) free(bm->bm_pixrect);
	if ((bm->bm_pixrect =
		mem_create(bm->bm_width, bm->bm_height, bm->bm_depth)) == NULL)
	   error("Create_Bitmap: cannot make pixrect (malloc failed)\n", 0, 0);
/*
 * BMAP_TYPE_UNPACKED - each pixel is in a different byte
 * BMAP_TYPE_PACKED   - pixels are packed tightly, byte boundaries are ignored
 * Depending on which type, the memory pixrect is filled in from the
 * data area accordingly.
 */
	switch (bm->bm_type)
	  {
	   case BMAP_TYPE_UNPACKED:		/* one pixel per byte */
	  {
	   FAST uchar *prptr;
	   FAST int x, y, w;

	   prptr = (uchar *) mpr_d(bm->bm_pixrect)->md_image;
	   if ((w = bm->bm_width) & 1) ++w;	/* round to word boundary */
	   for (y = 0; ++y <= bm->bm_height;)	/* for all Y's */
	     {
	      for (x = 0; ++x <= bm->bm_width;)	/* for all X's */
		 prptr[x] = *data++;		/* copy pixel */
	      prptr += w;			/* -> next scan line */
	     }
	   break;
	  }

	   case BMAP_TYPE_PACKED:		/* depth pixels per byte */
	   if (bm->bm_depth != 1)
	     {
	      free(bm->bm_pixrect);		/* trash the pixrect */
	      error("Create_Bitmap: PACKED format only allowed for depth 1\n", 0, 0);
	     }
	   bcopy(data, mpr_d(bm->bm_pixrect)->md_image,
		bm->bm_height * (bm->bm_width + 15) / 16);
	   break;
	  }
	return (bm->bm_pixrect);
}

/****
 *
 * Load_Bitmap(bmobj, data)
 * Load the bitmap data into the given buffer.
 *
 * notes:
 * If there is no pixrect or data for the bitmap, zeros are loaded.
 *
 ****/
Load_Bitmap(bmobj, data)
/*    --needs--			 */
	OBJID	bmobj;		/* BITMAP object */
FAST	uchar	*data;		/* -> data area */
{
/*    --uses--			 */
FAST	BMDATA	*bm;

	if ((bm->bm_width <= 0) || (bm->bm_height <= 0) ||
	    (bm->bm_depth <= 0)) return;
	switch (bm->bm_depth)
	  {
	   case 1:				/* PACKED */
	   bcopy(mpr_d(bm->bm_pixrect)->md_image, data,
		bm->bm_height * (bm->bm_width + 15) / 16);
	   break;

	   default:				/* UNPACKED */
	  {
	   FAST uchar *prptr;
	   FAST int x, y, w;

	   prptr = (uchar *) mpr_d(bm->bm_pixrect)->md_image;
	   if ((w = bm->bm_width) & 1) ++w;	/* round to word boundary */
	   for (y = 0; ++y <= bm->bm_height;)	/* for all Y's */
	     {
	      for (x = 0; ++x <= bm->bm_width;)	/* for all X's */
		 *data++ = prptr[x];		/* copy pixel */
	      prptr += w;			/* -> next scan line */
	     }
	   break;
	  }
	  }
}

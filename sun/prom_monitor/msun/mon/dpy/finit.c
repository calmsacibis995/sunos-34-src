/*
 *	@(#)finit.c 2.27 84/12/06 Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "../h/s2addrs.h"
#include "../h/globram.h"
#include "../h/dpy.h"

#ifdef S2COLOR
#include "../../sys/sun/fbio.h"
#endif S2COLOR


#ifdef S2FRAMEBUF
#include "../h/video.h"
#else  S2FRAMEBUF
#include "../h/framebuf.h"
#endif S2FRAMEBUF

#ifdef VECTORS
extern short bres[];
#endif VECTORS

extern unsigned char f_bitmap[], f_index[], f_data_hi[], f_data_lo[];

#ifdef COPYMODE
#include "../h/pginit.h"
#include "../h/s2map.h"

struct videoctl initial_video = {1, 1, 0, 0, 0}; /* Video & copy on, no int */

struct pginit s2fbtable[] = {
	{VIDEOMEM_BASE, 1, 
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
			PM_MEMORY, 0, 0, 0}},
	{VIDEOMEM_BASE+128*1024, PGINITEND, 
		{1, PMP_NONE, PM_MEMORY, 0, 0, 0}},
};
#endif COPYMODE


finit(newx, newy)
	unsigned newx, newy;
{
#ifdef COPYMODE
	struct pginit inittab[2];
#endif COPYMODE
#ifdef S2FRAMEBUF
	struct videoctl copyvid;
#endif S2FRAMEBUF
	int jumper_says_1024;

	/* Expand the compressed font into RAM */
	fexpand (font, f_bitmap, FBITMAPSIZE, f_index, f_data_hi, f_data_lo);

#ifdef S2FRAMEBUF

#ifdef COPYMODE
	/*
	 * Kludge to map the frame buffer in -- but only once!
	 *
	 * Check is that MemSize is not clobberred, and that it is
	 * greater than or equal to MemorySize.  If it is less than
	 * MemorySize, this means that MemorySize has been recomputed
	 * since we grabbed memory, so we should do it all again.
	 *
	 * This could be cleaned up (how much?) by moving it into inline
	 * memory mapping code (maybe). -- but you still have to remember
	 * on K1's and boots and other memory map re-initializations
	 * whether and where we have allocated video memory.
	 */
	if (gp->g_gfxmemsize != ~gp->g_gfxmemsizecomp || gp->g_gfxmemsize < gp->g_memorysize) {
		gp->g_memorysize -= 128*1024;	/* Steal 128K from the world */
		gp->g_memorysize &= -(128*1024);	/* Must be on 128K boundary */
		gp->g_gfxmemsize = gp->g_memorysize;
		gp->g_gfxmemsizecomp = ~gp->g_gfxmemsize;
	}
	inittab[0] = s2fbtable[0];
	inittab[1] = s2fbtable[1];
	inittab[0].pi_pm.pm_page = gp->g_gfxmemsize >> BYTES_PG_SHIFT;
	setupmap(inittab);	/* Fill in the map entries */

	/* Copy mode is used on old video where control reg is unreadable */
	copyvid = initial_video;
	copyvid.vc_copybase = gp->g_gfxmemsize >> VIDEO_COPY_SHIFT;
#else  COPYMODE
	copyvid = *VIDEOCTL_BASE;	/* Read current video controls */
	copyvid.vc_video_en = 1;	/* Enable video output */
#endif COPYMODE
	*VIDEOCTL_BASE = copyvid;

#ifdef S2COLOR
	/*
	 * Apologies for the convoluted logic here.
	 * We are testing to see if we should use color or b&w.
	 * If the color jumper is in, call init_scolor.
	 * If it says "no color board", use b&w.
	 * Otherwise, its result says whether color is 1024*1024.
	 */
	gp->g_fbtype = FBTYPE_SUN2BW;
	if (VIDEOCTL_BASE->vc_color_jumper &&
	    (jumper_says_1024 = init_scolor()) >= 0) {
		gp->g_fbtype = FBTYPE_SUN2COLOR;
	} else {
		jumper_says_1024 = VIDEOCTL_BASE->vc_1024_jumper;
	}
#else  S2COLOR
#ifdef FBBOTH
	jumper_says_1024 = VIDEOCTL_BASE->vc_1024_jumper;
#endif FBBOTH
#endif S2COLOR

#ifdef FBBOTH
	/*
	 * Decide whether we are 1024*1024 or 1192*900 and/or color.
	 */
	if (jumper_says_1024) {
		SCRWIDTH = 1024;
		SCRHEIGHT = 1024;
		WINTOP = 16+224/2;	/* Like Sun-1 but centered in 1024 */
		WINLEFT	= 16;
	} else {
		SCRWIDTH = 1152;
		SCRHEIGHT = 900;
		WINTOP = 56;
		WINLEFT	= 64;
	}

#endif FBBOTH

	/*
	 * Set up all the various garbage for the rasterop (pixrect) code.
	 */
	GXBase = (int)VIDEOMEM_BASE;
	fbdata.md_linebytes = SCRWIDTH/8;
/*	fbdata.md_image = GXBase;  they are identical */
	fbdata.md_offset.x = 0;
	fbdata.md_offset.y = 0;
/*	fbdata.md_primary is unreferenced */

/*	fbpr.pr_ops is unreferenced */
/*	fbpr.pr_size.x is unreferenced */
/*	fbpr.pr_size.y is unreferenced */
/*	fbpr.pr_depth is unreferenced */
	fbpr.pr_data = (caddr_t)&fbdata;

	fbpos.pr = &fbpr;
/*	fbpos.pos.x is set up by the call to pos() below */
/*	fbpos.pos.y is set up by the call to pos() below */

	chardata.md_linebytes = 2;
/*	chardata.md_image is filled in at run time */
	chardata.md_offset.x = 0;
	chardata.md_offset.y = 0;
/*	chardata.md_primary is unreferenced */

/*	charpr.pr_ops is unreferenced */
	charpr.pr_size.x = CHRWIDTH;
	charpr.pr_size.y = CHRHEIGHT;
/*	charpr.pr_depth is unreferenced */
	charpr.pr_data = (caddr_t)&chardata;

	charpos.pr = &charpr;
	charpos.pos.x = 0;
	charpos.pos.y = 0;
	
	/* Sun-2 0-bits are white, 1-bits are black. */
	fillfunc = POX_CLR;			/* White screen */
	chrfunc = POX_SRC;			/* No inverse video */
#ifdef VECTORS
	vectfunc = POX_SET;			/* Draw, don't undraw, vecs */
#endif VECTORS

#else  S2FRAMEBUF

	/* Sun-1 0-bits are black, 1-bits are white. */
	fillfunc = POX_SET;			/* White screen */
	chrfunc = POX_NOT(POX_SRC);		/* No inverse video */
#ifdef VECTORS
	vectfunc = POX_CLR;			/* Draw, don't undraw, vecs */
#endif VECTORS

	GXcontrol = GXvideoEnable;		/* Turn on screen */

#endif S2FRAMEBUF

	state = ALPHA;
	cursor = BLOCKCURSOR;
	scrlins = 1;				/* scroll by 1's initially */

#ifdef VECTORS
	yloseen = 0;
	pendown = 0;				/* Don't draw vectors */
	gx = 0; gy = 0;				/* home graph cursor */
	gxhi = 0; gxlo = 0;			/* home graph regs */
	gyhi = 0; gylo = 0;
	{
		register int i;
		for (i = 0; i < BRESIZ; i++)	/* Bresenham commutes to his */
			bresw(i) = bres[i];	/* place of work */
	}
#endif VECTORS

	pos(newx, newy);			/* Set ax, ay, dcok */
}

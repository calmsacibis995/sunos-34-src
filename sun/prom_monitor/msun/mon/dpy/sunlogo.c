/*
 *	@(#)sunlogo.c 1.6 84/08/08 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "../h/globram.h"
#include "../h/dpy.h"

#ifndef S2FRAMEBUF
#include "../h/framebuf.h"
#endif S2FRAMEBUF

static unsigned	logo_data[128] = {
			0x00000003, 0xC0000000, 0x0000000F, 0xF0000000,
			0x0000001F, 0xF8000000, 0x0000003F, 0xFC000000,
			0x0000003F, 0xFE000000, 0x0000007F, 0xFF000000,
			0x0000007E, 0xFF800000, 0x0000027F, 0x7FC00000,
			0x0000073F, 0xBFE00000, 0x00000FBF, 0xDFF00000,
			0x00001FDF, 0xEFF80000, 0x00002FEF, 0xF7FC0000,
			0x000077F7, 0xFBFE0000, 0x0000FBFB, 0xFDFF0000,
			0x0001FDFD, 0xFEFF0000, 0x0001FEFE, 0xFF7EC000,
			0x0006FF7F, 0x7FBDE000, 0x000F7FBF, 0xBFDBF000,
			0x001FBFDF, 0xDFE7F800, 0x003FDFEF, 0xEFEFF400,
			0x007FAFF7, 0xF7DFEE00, 0x00FF77FB, 0xF3BFDF00,
			0x01FEFBFD, 0xF97FBF80, 0x03FDFDFF, 0xF8FF7F00,
			0x07FBF8FF, 0xF9FEFE00, 0x0FF7F07F, 0xF3FDFCE0,
			0x1FEFE73F, 0xF7FBFBF8, 0x3FDFDFDF, 0xEFF7F7FC,
			0x7FBFBFE7, 0x9FEFEFFE, 0x7F7F7FE0, 0x1FDFDFFE,
			0xFEFEFFF0, 0x3FBFBFFF, 0xFDFDFFF0, 0x3F7F7FFF,
			0xFFFBFBF0, 0x3FFEFF7F, 0xFFF7F7F0, 0x3FFDFEFF,
			0x7FEFEFE0, 0x1FFBFDFE, 0x7FDFDFE7, 0x9FF7FBFE,
			0x3FBFBFDF, 0xEFEFF7FC, 0x0E7F7FBF, 0xF39FEFF8,
			0x00FEFF3F, 0xF83FDFF0, 0x01FDFE7F, 0xFC7FBFE0,
			0x03FBFC7F, 0xFEFF7FC0, 0x01F7FA7E, 0xFF7EFF80,
			0x00EFF73F, 0x7FBDFF00, 0x005FEFBF, 0xBFDBFE00,
			0x003FDFDF, 0xDFE7FC00, 0x001FAFEF, 0xEFF7F800,
			0x000F77F7, 0xF7FBF000, 0x0006FBFB, 0xFBFDE000,
			0x0001FDFD, 0xFDFEC000, 0x0001FEFE, 0xFEFF0000,
			0x0000FF7F, 0x7F7F0000, 0x00007FBF, 0xBFBE0000,
			0x00003FDF, 0xDFDC0000, 0x00001FEF, 0xEFE80000,
			0x00000FF7, 0xF7F00000, 0x000007FB, 0xFBE00000,
			0x000003FD, 0xF9C00000, 0x000001FE, 0xFC800000,
			0x000000FF, 0xFC000000, 0x0000007F, 0xFC000000,
			0x0000003F, 0xF8000000, 0x0000001F, 0xF8000000,
			0x0000000F, 0xF0000000, 0x00000003, 0xC0000000
		};

/*
 *	Draw the Sun logo, starting on line (y) on screen.
 *	No clipping, y must be valid.
 */

#ifdef S2FRAMEBUF

sunlogo(y)
	unsigned short y;
{
	register long *addr, *logo;
	register short i;
	
	addr = (long *) (GXBase +
		(unsigned short)((WINTOP + y * CHRHEIGHT)) * 
		(unsigned short)(SCRWIDTH/8)
		+ 2*(WINLEFT/16) );
	logo = (long *) logo_data;
	for (i = 64; i-- != 0;) {
		if (chrfunc == POX_SRC) {
			*addr++ = *logo++;
			*addr++ = *logo++;
		} else {
			*addr++ = ~*logo++;
			*addr++ = ~*logo++;
		}
		addr += (SCRWIDTH/32) - 2;	/* -2 for the autoincs */
	}
}

#else  S2FRAMEBUF

sunlogo(y)
	short y;
{
	register unsigned short *yy, *xx, *logo;
	unsigned short *origxx;
	register short i;

	GXwidth = 16;
	GXfunction = chrfunc;
	yy = (unsigned short *)
		(GXBase+GXselecty
		 +(WINTOP<<1)+y*(CHRHEIGHT<<1));
	origxx = (unsigned short *)
		(GXBase+GXselectX+GXupdate+GXsource
		 +(WINLEFT<<1));
	logo = (unsigned short *)logo_data;
	for (i = 64; i-- != 0;) {
		xx = origxx, *yy++ = 1;
		*xx = *logo++; xx += 16;
		*xx = *logo++; xx += 16;
		*xx = *logo++; xx += 16;
		*xx = *logo++; xx += 16;
	}
}
#endif S2FRAMEBUF

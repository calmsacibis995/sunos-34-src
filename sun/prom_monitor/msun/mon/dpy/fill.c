/*
 *	@(#)fill.c 2.8 84/08/07 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "../h/globram.h"
#include "../h/dpy.h"

#ifndef S2FRAMEBUF
#include "../h/framebuf.h"
#endif S2FRAMEBUF

#ifdef S2FRAMEBUF

/*
 * Fill a rectangle on the screen with black or white.
 *
 * For the Sun-2 without RasterOp support, only works on word boundaries.
 */
rectfill(xlo,ylo,xhi,yhi)
{
	int yy, width, filler;
	unsigned short y;

	width = (xhi - xlo) >> 3;	/* Bytes of difference */
	filler = (fillfunc == POX_SET)? -1: 0;
	y = ylo;
	yy = GXBase + (y * (unsigned short)(SCRWIDTH >> 3)) + (xlo >> 3);
	for (;
	     y < yhi;
	     y++, yy += SCRWIDTH >> 3) {
		setshort(yy, yy+width, filler);
	}
}


/*
 * Invert the entire screen.
 */
invertwholescreen()
{
	register long *q, *end;
	register long invert = -1;

	q = (long *)GXBase;
	end = (long *) (GXBase + 128*1024);

	for (; q < end; q++) *q ^= invert;
}

#else  S2FRAMEBUF

rectfill(xlo,ylo,xhi,yhi)
{
	int x, tmp;

	xhi -= 16;	/* Since that's how we always use it */

	for (x = xlo; x <= xhi/*-16*/; x += 16) {
		GXfunction = fillfunc;
		GXwidth = 16;
		colfill(x,ylo,yhi);
	}
	if (tmp = (xhi/*-16*/ - xlo) & 15) {
		GXfunction = fillfunc;
		GXwidth = tmp;
		colfill(x,ylo,yhi);
	}
}

colfill(x,ylo,yhi)
{

	*(unsigned short *)(GXBase+(x<<1)) = 1;
	setshort( (GXBase|GXsource|GXupdate|GXselectY)+(ylo<<1),
		  (GXBase|GXsource|GXupdate|GXselectY)+(yhi<<1));
}

invertwholescreen()
{
	int x;

	for (x = 0; x < SCRWIDTH; x += 16) {
		GXfunction = POX_NOT(POX_DST);
		GXwidth = 16;
		colfill(x,0,SCRHEIGHT);
	}
}
#endif S2FRAMEBUF

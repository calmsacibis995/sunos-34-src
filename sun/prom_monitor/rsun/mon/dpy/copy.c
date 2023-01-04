/*
 *	@(#)copy.c 2.8 84/08/07 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "../h/globram.h"
#include "../h/dpy.h"

#ifndef S2FRAMEBUF
#include "../h/framebuf.h"
#endif S2FRAMEBUF

#ifdef S2FRAMEBUF

/*
 * Copy one rectangle in the frame buffer to another.
 * 
 * On the Sun-2, without RasterOp support, this only works word-aligned.
 */
rectcopy(fromxlo,fromylo,fromxhi,fromyhi,tox,toy)
	int fromxlo, fromylo, fromxhi, fromyhi, tox, toy;
{
	int y, yy, toyy;
	int width, height;
	
	if (fromxlo < 0) fromxlo = 0;
	if (fromylo < 0) fromylo = 0;
	if (fromxhi > SCRWIDTH) fromxhi = SCRWIDTH;
	if (fromyhi > SCRHEIGHT) fromyhi = SCRHEIGHT;
	if (tox < 0) tox = 0;
	if (toy < 0) toy = 0;

	yy =   GXBase + (fromxlo >> 3) +
		((unsigned short)fromylo * (unsigned short)(SCRWIDTH >> 3));
	toyy = GXBase + (tox     >> 3) +
		((unsigned short)toy     * (unsigned short)(SCRWIDTH >> 3));
	width = (fromxhi - fromxlo) >> 3;

	if (fromylo >= toy) {
		for (y = fromylo;
		     y < fromyhi;
		     y++, yy += SCRWIDTH>>3, toyy += SCRWIDTH>>3) {
			bltshort(yy, yy+width, toyy);
		}
	} else {
		height = ((unsigned short)(fromyhi - fromylo)) * 
			  (unsigned short)(SCRWIDTH >> 3);
		yy += height;
		toyy += height;
		for (y = fromyhi-1;
		     y >= fromylo;
		     y--, yy -= SCRWIDTH>>3, toyy -= SCRWIDTH>>3) {
			bltshort(yy, yy+width, toyy);
		}
	}
}

#else  S2FRAMEBUF

rectcopy(fromxlo,fromylo,fromxhi,fromyhi,tox,toy)
	int fromxlo, fromylo, fromxhi, fromyhi, tox, toy;
{
	register int x, xx, tmp;

	if (fromxlo < 0) fromxlo = 0;
	if (fromylo < 0) fromylo = 0;
	if (fromxhi > SCRWIDTH) fromxhi = SCRWIDTH;
	if (fromyhi > SCRHEIGHT) fromyhi = SCRHEIGHT;
	if (tox < 0) tox = 0;
	if (toy < 0) toy = 0;

	fromxhi -= 16;  /* Since that's how we use it ever'where */

	if (fromxlo >= tox)
		for (x = fromxlo, xx = tox;
		     x <= fromxhi/*-16*/;
		     x+=16, xx+=16) {
			GXfunction = GXSOURCE;
			GXwidth = 16;
			colcopy(x,fromylo,fromyhi,xx,toy);
	} else {
		for (x = fromxhi/*-16*/, xx = tox+(fromxhi-fromxlo)/*-16*/;
		     x >= fromxlo;
		     x-=16, xx-=16) {
			GXfunction = GXSOURCE;
			GXwidth = 16;
			colcopy(x,fromylo,fromyhi,xx,toy);
		}
		x = fromxlo;	/* since < 16 bits is left justified */
		xx = tox;
	}
	tmp = (fromxhi/*-16*/ - fromxlo) & 15;
	if (tmp > 0) {
		GXfunction = GXSOURCE;
		GXwidth = tmp;
		colcopy(x,fromylo,fromyhi,xx,toy);
	}
}

colcopy(fromx,fromylo,fromyhi,tox,toy)
{
	int bltrshort(), bltshort();
	register (*whichblt) () = bltrshort;

	*(unsigned short *)(GXBase+fromx+fromx) = 1;
	*(unsigned short *)(GXBase+GXset1+tox+tox) = 1;
	/* We have to pick which BLT to use since the highorder bits
	   in our arguments screw up bltshort's decision making */
	if (fromylo >= toy) whichblt = bltshort;

	(*whichblt) ( GXBase+GXsource+GXselectY+        fromylo+fromylo,
		      GXBase+GXsource+GXselectY+        fromyhi+fromyhi,
		      GXBase+GXupdate+GXset1+GXselectY+ toy+toy );
}
#endif S2FRAMEBUF

/*
 *	@(#)drawchr.c 2.11 83/12/20 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "../h/globram.h"
#include "../h/dpy.h"

#ifndef S2FRAMEBUF
#include "../h/framebuf.h"
#endif S2FRAMEBUF

/*
 *	Draw the character pointed to (in the font) by  p
 *	at position  ax , ay  on the screen (global variables)
 *	using function f
 */

#ifdef S2FRAMEBUF

drawchr(p,f)
	short *p;
	int f;
{

	/* FIXME: maybe avoid multiplies in batchrop too, by fudging addr? */
	if (!dcok) {
		fbpos.pos.x = WINLEFT + ax * CHRWIDTH;
		fbpos.pos.y = WINTOP  + ay * CHRHEIGHT;
		dcok = 1;
	}
	chardata.md_image = p;
	prom_mem_batchrop(fbpos, f, &charpos, 1);
}

#else  S2FRAMEBUF

drawchr(p,f)
	register unsigned short *p;
	short f;
{
	register unsigned short *q;

	GXwidth = CHRWIDTH;
	GXfunction = f;
	if (!dcok) {
		q = (unsigned short *)
			(GXBase+GXselecty+GXupdate+GXsource
			 +(WINTOP<<1)+((char)ay)*(CHRHEIGHT<<1));
		dcay = q;
		dcax = (unsigned short *)
			(GXBase+GXselectX
			 +(WINLEFT<<1)+((char)ax)*(CHRWIDTH<<1));
		dcok = 1;
	} else
		q = dcay;	/* Set from saved value */

	*dcax = 1;  /* Set it in fb */
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p;
}
#endif S2FRAMEBUF

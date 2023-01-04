/*
 *	@(#)insdel.c 2.8 84/08/07 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "../h/globram.h"
#include "../h/dpy.h"
#define	WINBOT (WINTOP+BOTTOM*CHRHEIGHT)
#define	WINRIGHT (WINLEFT+RIGHT*CHRWIDTH)

/* delete lines a (inclusive) to b (exclusive) */
delline(a,b)
	unsigned char a, b;
{
	register pixla = a*CHRHEIGHT, pixlb = b*CHRHEIGHT;

	rectcopy(0,WINTOP+pixlb,		SCRWIDTH,WINBOT,
		 0,WINTOP+pixla);
	rectfill(0,WINBOT-(pixlb-pixla),	SCRWIDTH,SCRHEIGHT);
}

/* insert (make room for) lines a (inclusive) to b (exclusive) */
insline(a,b)
	unsigned char a, b;
{
	register pixla = a*CHRHEIGHT, pixlb = b*CHRHEIGHT;

	rectcopy(0,WINTOP+pixla,    SCRWIDTH,WINBOT-(pixlb-pixla),
		 0,WINTOP+pixlb);
	rectfill(0,WINTOP+pixla,    SCRWIDTH,WINTOP+pixlb);
}

#ifdef S2FRAMEBUF
/*
 * delete chars a (inclusive) to b (exclusive) on line l
 *
 * One by one, we move any characters that need to slide.
 * We fill one character position by drawing a blank there,
 * then we know that rounding the cursor position down to a 16-bit boundary
 * is good enough to fill the rest of the line.
 */
delchar(a, b, l)
	unsigned char a, b, l;
{
	int savex = ax;
	int savey = ay;
	register int apos, bpos;
	short achar[CHRSHORTS];
	
	if (b < RIGHT) {
		/*
		 * We have to actually shift characters.
		 */
		fbpos.pos.y = WINTOP + l * CHRHEIGHT;	/* All on this line */
		apos = WINLEFT + a * CHRWIDTH;
		bpos = WINLEFT + b * CHRWIDTH;
		chardata.md_image = achar;	/* Here's temp char location */
		for (;bpos < WINRIGHT;
		      apos += CHRWIDTH, bpos += CHRWIDTH) {
			fbpos.pos.x = bpos;
			prom_mem_grab(fbpos, 0, &charpos, 0);
			fbpos.pos.x = apos;
			prom_mem_batchrop(fbpos, PIX_SRC, &charpos, 1);
		}
		ax = RIGHT-(b-a);	/* Position to fill from */
	} else {
		ax = a;			/* Position to fill from */
	}

	ay = l;
	dcok = 0;		/* We're doing severe play with dev. coords */
	drawchr(font, fillfunc);	/* Fill one char position there */
	
	/* OK, that character has been filled; now do the rest. */
	rectfill( (fbpos.pos.x + CHRWIDTH) &~ 0x000F,
		 fbpos.pos.y, 
		 SCRWIDTH,
		 fbpos.pos.y+CHRHEIGHT);
	
	pos(savex, savey);		/* Clean up our grunge */
}
#else  S2FRAMEBUF
/* delete chars a (inclusive) to b (exclusive) on line l */
delchar(a,b,l)
	unsigned char a, b, l;
{
	register pixl = l*CHRHEIGHT, pixa = a*CHRWIDTH, pixb = b*CHRWIDTH;

	rectcopy(WINLEFT+pixb,	WINTOP+pixl, 
		 WINRIGHT,	WINTOP+CHRHEIGHT+pixl, 
		 WINLEFT+pixa,	WINTOP+pixl);

	rectfill(WINRIGHT-(pixb-pixa),WINTOP+pixl, 
		 SCRWIDTH,	WINTOP+CHRHEIGHT+pixl);
}
#endif S2FRAMEBUF

#ifdef S2FRAMEBUF
/* insert (make room for) chars a (inclusive) to b (exclusive) on line l */
inschar(a, b, l)
	unsigned char a, b, l;
{
	int savex = ax;
	int savey = ay;
	register int apos, bpos;
	register short temp;
	short achar[CHRSHORTS];
	
	if (b > RIGHT) b = RIGHT;		/* Avoid problems */

	fbpos.pos.y = WINTOP + l * CHRHEIGHT;	/* All on this line */
	bpos = WINRIGHT - CHRWIDTH;		/* Start on last char */
	apos = bpos - (b - a) * CHRWIDTH;	/* Copy from N chars back */
	chardata.md_image = achar;	/* Here's temp char location */
	temp = RIGHT - b;			/* How many to copy */

	for (;temp-- != 0;
	      apos -= CHRWIDTH, bpos -= CHRWIDTH) {
		fbpos.pos.x = apos;
		prom_mem_grab(fbpos, 0, &charpos, 0);
		fbpos.pos.x = bpos;
		prom_mem_batchrop(fbpos, PIX_SRC, &charpos, 1);
	}

	ax = a; ay = l;		/* Position to fill from */
	dcok = 0;		/* We're doing severe play with dev. coords */
	for (; a < b; a++, fbpos.pos.x += CHRWIDTH) {
		drawchr(font, fillfunc);	/* Fill one char position */
	}

	pos(savex, savey);
}
#else  S2FRAMEBUF
/* insert (make room for) chars a (inclusive) to b (exclusive) on line l */
inschar(a,b,l)
	unsigned char a, b, l;
{
	register pixl = l*CHRHEIGHT, pixa = a*CHRWIDTH, pixb = b*CHRWIDTH;

	rectcopy(WINLEFT+pixa,	WINTOP+pixl, 
		 WINRIGHT-(pixb-pixa),WINTOP+CHRHEIGHT+pixl,
		 WINLEFT+pixb,	WINTOP+pixl);

	rectfill(WINLEFT+pixa,	WINTOP+pixl,
		 WINLEFT+pixb,	WINTOP+CHRHEIGHT+pixl);
}
#endif S2FRAMEBUF

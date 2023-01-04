/*
 *	@(#)vec.c 2.5 83/09/16 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "../h/globram.h"
#include "../h/framebuf.h"
#include "../h/dpy.h"

#define MOV(reg,sign) (0x3000|reg|sign)  /* builds a movw instruction */
#define RX 0x800	/* a4 holds rx address */
#define RY 0x600	/* a3 holds ry address */
#define POS 0xc0	/* an@+ scans positively */
#define NEG 0x100	/* an@- scans negatively */

unsigned short mincmd[8] = {
	MOV(RY,POS), MOV(RX,POS),
	MOV(RY,POS), MOV(RX,NEG),
	MOV(RY,NEG), MOV(RX,POS),
	MOV(RY,NEG), MOV(RX,NEG)
};

unsigned short majcmd[8] = {
	MOV(RX,POS), MOV(RY,POS),
	MOV(RX,NEG), MOV(RY,POS),
	MOV(RX,POS), MOV(RY,NEG),
	MOV(RX,NEG), MOV(RY,NEG)
};

vec(x0,y0,x1,y1)
	unsigned x0,y0,x1,y1;
{
	/* NOTE!  ATTEN-SHUN!  

	These register variables are declared this way and in this order
	because the bresenham assembly code fragment ASSUMES that certain
	values are in certain registers.  DO NOT change any of the 
	declarations in this module UNLESS you're willing to fix it
	when it breaks.

	END OF REMINDER/WARNING  -- JCGilmore, 20 June 1982 */

	register struct globram *G;
	register x, y, q = 0, r;
	register unsigned t = 0;
	register unsigned short *rx, *ry;

	if ( (x0|y0|x1|y1) >> 10 ) return;	/* test coords in [0,1024) */

	GXwidth = 1;

	rx = (unsigned short*)(GXBase|GXsource|GXselectx) + x0;
	ry = (unsigned short*)(GXBase|GXsource|GXselecty) + y0;

	x = x1 - x0;				/* x,y relative to x0,y0 */
	y = y1 - y0;

	if (x<0) {
		x = -x;
		rx++;		/* since an@- predecrements */
		q += 2;
	}

	if (y<0) {
		y = -y;
		ry++;		/* since an@- predecrements */
		q += 4;
	}

	if (x<y) {
		t = x;
		x = y;
		y = t;
		q += 1;
		ry = (unsigned short *)( (int)ry | GXupdate);
	}
	else
		rx = (unsigned short *)( (int)rx | GXupdate);

	bresw(BRESMINOR) = mincmd[q];
	bresw(BRESMAJOR) = majcmd[q];
	q = x;
	r = -(x>>1);
	(*(int (*)())(&bresw(0)))();	/* call Bresenham */
	return;
}

/*
 *	@(#)vector.c 2.8 84/01/05 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * This module, vector.c, contains control code handlers for
 * graphic control sequences.  It emulates the Tektronix 4014,
 * after a fashion.  It is called by fwritechar if it doesn't
 * know how to deal with the current state of the terminal.  (eg
 * GRAPH mode).  If this routine is not present, the terminal
 * reverts to ALPHA state and tries again.
 */

#include "../h/framebuf.h"
#include "../h/sunromvec.h"
#include "../h/globram.h"
#include "../h/dpy.h"

#define CTRL(c) ('c'-64 & 127)

/* Interpret a single character in a graphics mode. */

int
fwrvect(c)
	unsigned char c;
{

	switch (state) {

	case GRAPH:
		return plotv(c,1);

	case PNT:
		return plotv(c,0);

	case INC:
		switch (c) {
			case ' ':	pendown = 0;		break;
			case 'P':	pendown = 1;		break;
			case 'A':	inc(1,0);		break;
			case 'E':	inc(1,1);		break;
			case 'D':	inc(0,1);		break;
			case 'F':	inc(-1,1);		break;
			case 'B':	inc(-1,0);		break;
			case 'J':	inc(-1,-1);		break;
			case 'H':	inc(0,-1);		break;
			case 'I':	inc(1,-1);		break;
			case CTRL([):	state |= ESC;		break;
			case CTRL(_):
			case CTRL(M):
					cursor = BLOCKCURSOR;
					state = ALPHA;
					return -1;
			default:	/* ignore */		break;
		}
		break;

	} /* End of switch (state) */
	return 0;
}


/*
 * Plot in vector or point modes.  Interprets received characters
 * to generate vectors or whatever.
 *
 * Result is 0 if character was interpreted, -1 if it needs to be
 * reinterpreted in ALPHA mode.  (i.e. we have left grafix mode).
 *
 * Arg  mid  is 1 for vector plotting and 0 for point plotting.
 */
plotv(c,mid)
	unsigned char c;
	char mid;
{
	int tmp = (c & 31) << 2;

	switch (c>>5) {
		case 0:		switch (c) {
				default:	/* ignore */	break;
				case CTRL(G):	if (pendown)
							blinkscreen();
						else    pendown = 1;
								break;
				case CTRL([):	state |= ESC;	break;

				case CTRL(M):
				case CTRL(\\):
				case CTRL(]):
				case CTRL(^):
				case CTRL(_):	/* Pass back to alpha mode */
						cursor = BLOCKCURSOR;
						state = ALPHA;
						return -1;
				}				break;
		case 1:		tmp <<= 5;
				if (yloseen) gxhi = tmp;
				else gyhi = tmp;		break;
		case 2:		gxlo = tmp + (gxlo&3);
				yloseen = 0;
				if (!mid) pendown = 0;
				writev();
				pendown = 1;
				if (!mid) writev();		break;
		case 3:		if (!yloseen) {
					gylo = tmp + (gylo&3);
					yloseen = 1;
				}
				else {
					gylo = tmp+((gylo>>4)&3);
					gxlo = (gxlo&-4)+((gylo>>2)&3);
				}				break;
	}
	return 0;
}


/* Write a vector on the screen, based on pendown and Lo/Hi coordinates */

writev()
{
	register gxnew = (gxhi+gxlo) >> 2, gynew = (gyhi+gylo) >> 2;

	if (pendown) {
		GXfunction = vectfunc;
		vec(gx,780-gy,gxnew,780-gynew);
	}
	gx = gxnew; gy = gynew;
	/* A future speedup could avoid doing these divides on each vector,
	   and only do it on entry into alpha mode. */
	pos (gx/CHRWIDTH, ((780-CHRHEIGHT)-gy)/CHRHEIGHT);
}


/* Perform an incremental plot mode move; just simulate "real" plot mode */

inc(dx,dy)
{

	gxlo += dx;
	gxhi += gxlo & ~127;
	gxlo &= 127;
	gylo += dy;
	gyhi += gylo & ~127;
	gylo &= 127;
	writev();
}

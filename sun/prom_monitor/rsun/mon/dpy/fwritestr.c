/*
 *	@(#)fwritestr.c 2.20 84/03/19 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Interpret a string of characters of length <len> into the frame buffer.
 *
 * Note that characters with the high bit set will not be recognized.
 * This is good, for it reserves them for ASCII-8 X3.64 implementation.
 * It just means all sources of chars which might come here must mask
 * parity if necessary.
 */

#include "../h/globram.h"
#include "../h/dpy.h"

#ifdef KEYBS2
#include "../h/keyboard.h"
#endif KEYBS2

#ifdef S2FRAMEBUF
#include "../h/video.h"
#include "../h/s2addrs.h"
#else  S2FRAMEBUF
#include "../h/framebuf.h"
#endif S2FRAMEBUF

#define CTRL(c) ('c'-64 & 127)


fwritechar(c)
	unsigned char c;
{

	fwritestr(&c,1);
}


fwritestr(addr,len)
    register unsigned char *addr;
    register short len;		/* Declared short to get dbra - hah! */
{
    register char c;
    int lfs;			/* Lines to feed on a LF */

    cursorcomp();
    for (; --len != -1; ) {

	c = *addr++;		/* Fetch next char from string */

beginning:

	if (state & ESC) {
		switch (c) {
			case 0:		/*ignored*/		continue;
			case CTRL(J):	/*ignored*/		continue;
			case CTRL(M):	/*ignored*/		continue;
			case CTRL([):	/*ignored*/		continue;
			case CTRL(?):	/*ignored*/		continue;
			/* Begin X3.64 sequence; enter alpha mode */
			case '[':	state = ESCBRKT;
					cursor = BLOCKCURSOR;
								continue;
			/* Clear screen and enter alpha mode */
			case CTRL(L):	state = ALPHA;
					cursor = BLOCKCURSOR;
					pos(LEFT,TOP);
					rectfill(0,0,SCRWIDTH,SCRHEIGHT);
								continue;
			/* Simulate DEL char for systems that can't xmit it */
			case '?':	c = CTRL(?);
					state &= ~ESC;		break;

			/* By default, ignore the character after the ESC */
			default:	state &= ~ESC;
#ifdef VECTORS
					if (0x60 == (c & 0x60)) {
						/* Got beam/vec type select */
						/* For us, write-thru = undraw;
						   all else = draw solid vect */
						vectfunc = (0x10 == (c&0x18))?
							fillfunc:
							POX_NOT(fillfunc);
					}
#endif VECTORS
					continue;
		}
	}

	switch (state) {

	case ALPHA:
		ac = 0;
		ac0 = -1;
		switch (c) {
			case CTRL(G):	blinkscreen();		break;
			case CTRL(H):	pos(ax-1,ay);	 	break;
			case CTRL(I):	pos((ax&-8)+8,ay); 	break;
			case CTRL(J):	/* linefeed */
	/* Linefeed is complicated, so it gets its own indentation */
FeedLine:
	if (ay < BOTTOM-1) {
		/* pos(ax,ay+1); */
		ay++;
#ifdef S2FRAMEBUF
		fbpos.pos.y += CHRHEIGHT;
#else  S2FRAMEBUF
		dcay += CHRHEIGHT;
#endif S2FRAMEBUF
		if (!scrlins) /* ...clear line */
			delchar(0,RIGHT,ay);
	} else {
		if (!scrlins) {  /* Just wrap to top of screen and clr line */
			pos (ax,0);
			delchar(0,RIGHT,ay);
		} else {
			lfs = scrlins;	/* We will scroll, but how much? */
			if (lfs == 1) {
				/* Find pending LF's and do them all now */
				unsigned char *cp = addr;
				short left = len;

				for (; --len != -1; addr++) {
					     if (*addr == CTRL(J)) lfs++;
					else if (*addr == CTRL(M)) ;
					else if (*addr >= ' ')     ;
					else if (*addr > CTRL(J)) break;
				}
				len = left; addr = cp;
			}
			if (lfs > BOTTOM) lfs = BOTTOM;
			delline (TOP, TOP+lfs);
			if (lfs != 1) /* avoid upsetting <dcok> for nothing */
			    pos (ax, ay+1-lfs);
		}
	}
	break;
			case CTRL(K):	pos(ax,ay-1); /* 4014 */	break;
			case CTRL(L):	pos(LEFT,TOP);
					rectfill(0,0,SCRWIDTH,SCRHEIGHT);
			case CTRL(M):	/* pos(0,ay); */
					ax = 0;
#ifdef S2FRAMEBUF
					fbpos.pos.x = WINLEFT;
#else  S2FRAMEBUF
					dcax = (unsigned short *)
					    (GXBase+GXselectX +(WINLEFT<<1));
#endif S2FRAMEBUF
					break;
			case CTRL([):	state |= ESC; 		break;
#ifdef VECTORS
			case CTRL(\\):	state = PNT;
					cursor = NOCURSOR;	break;
			case CTRL(]):	state = GRAPH;
					cursor = NOCURSOR;
					pendown = 0;		break;
			case CTRL(^):	state = INC;
					cursor = NOCURSOR;
					pendown = 1;		break;
#endif VECTORS
			case CTRL(?):	/* ignored */		break;

			default:

			    c -= 32;
			    if (c >= 0)
			    {
				/* Write character on the screen. */
				drawchr (font[c], chrfunc);
				/* Update cursor position.  Inline for speed. */
				if (ax < RIGHT-1) {
					ax++;
#ifdef S2FRAMEBUF
					fbpos.pos.x += CHRWIDTH;
#else  S2FRAMEBUF
					dcax += CHRWIDTH;
#endif S2FRAMEBUF
				} else {
					/* Wrap to col 1, pretend LF seen */
					ax = 0;
#ifdef S2FRAMEBUF
					fbpos.pos.x = WINLEFT;
#else  S2FRAMEBUF
					dcax = (unsigned short *)
					      (GXBase+GXselectX +(WINLEFT<<1));
#endif S2FRAMEBUF
					goto FeedLine;
				}
			    }
			    break;
		}  /* end of case ALPHA switch statement */
		break;

	case ESCBRKT:
		if ('0' <= c && c <= '9') {
			ac = ((char)ac)*10 + c - '0'; /* char for inline muls */
		} else if (c == ';') {
			ac0 = ac; ac = 0;
		} else {
		    acinit = ac;
		    if (ac == 0) ac = 1;	/* Default value is 1 */
		    switch ( c ) {
			case '@':	inschar(ax,ax+ac,ay);	break;
			case 'A':	pos(ax,ay-ac); 		break;
			case 'B':	pos(ax,ay+ac); 		break;
			case 'C':	pos(ax+ac,ay); 		break;
			case 'D':	pos(ax-ac,ay); 		break;
			case 'E':	pos(LEFT,ay+ac);		break;
			case 'f':
			case 'H':
					if (ac0 < 0) pos(0,ac-1);
					else         pos(ac-1,ac0-1);	break;
			case 'J':	delline(ay+1,BOTTOM); /* no break */
			case 'K':	delchar(ax,RIGHT,ay);		break;
			case 'L':	insline(ay,ay+ac); 		break;
			case 'M':	delline(ay,ay+ac);		break;
			case 'P':	delchar(ax,ax+ac,ay);		break;
			case 'm':	chrfunc = fillfunc^
					  ((acinit == 0)? 
						POX_SRC:
						POX_NOT(POX_SRC)
							      );	break;
#ifdef S2FRAMEBUF
			/* Sun-2 0-bits are white, 1-bits are black. */
			case 'p':	if (fillfunc != POX_CLR)
						screencomp();		break;
			case 'q':	if (fillfunc != POX_SET)
						screencomp();		break;
#else  S2FRAMEBUF
			/* Sun-1 0-bits are black, 1-bits are white. */
			case 'p':	if (fillfunc != POX_SET)
						screencomp();		break;
			case 'q':	if (fillfunc != POX_CLR)
						screencomp();		break;
#endif S2FRAMEBUF
			case 'r':	scrlins = acinit;		break;
			case 's':	finit(ax, ay);			break;
			default:	/* X3.64 sez ignore if we don't know */
					if ((c < '@')) {
						state = SKIPPING;
						continue;
					}
		    }
		state = ALPHA;
		}
		break;

	case SKIPPING:	/* Waiting for char from cols 4-7 to end esc string */
		if (c < '@') break;
		state = ALPHA;
		break;

	default:
		/* Deal with graphics states if 2nd prom exists, else just
		   go to ALPHA mode and reinterpret the character. */
#ifdef VECTORS
		if (fwrvect(c) < 0) goto beginning;
		break;
#else  VECTORS
		state = ALPHA;
		cursor = BLOCKCURSOR;
		goto beginning;
#endif VECTORS
	}
    }			/* End of for loop thru string of chars */

    cursorcomp();	/* Restore the cursor to the screen */
}


pos(x,y)
	int x, y;
{

	dcok = 0;	/* We've changed ax or ay and not dcax,dcay */

	ax = x < LEFT?		LEFT:
	     x >= RIGHT?	RIGHT-1:
				x;

	ay = y < TOP? 		TOP:
	     y >= BOTTOM?	BOTTOM-1:
				y;
}


cursorcomp()
{

	if (cursor != NOCURSOR)
/* The 2nd param (font) is used as the source for the character sent to the
   frame buffer.  However, the function we are using ignores the source,
   so we just punt with  font . 	*/
		drawchr(font, POX_NOT(POX_DST));
}

screencomp()
{

#ifndef S2FRAMEBUF
	GXcontrol = ~GXvideoEnable;
#endif S2FRAMEBUF
	invertwholescreen();
#ifndef S2FRAMEBUF
	GXcontrol = GXvideoEnable;
#endif S2FRAMEBUF
	chrfunc = POX_NOT(chrfunc);
	fillfunc = POX_NOT(fillfunc);
#ifdef VECTORS
	vectfunc = POX_NOT(vectfunc);
#endif VECTORS
}

blinkscreen()
{
#ifdef S2FRAMEBUF
	struct videoctl copyvid;
#endif S2FRAMEBUF
	int i = 10000;

#ifdef KEYBS2
	if (gp->g_insource == INKEYB) {
		/* Sun-2 keyboard: Ring the bell on the keyboard */
		while (!sendtokbd(KBD_CMD_BELL)) if (0 == --i) return;
		while (--i);
		(void) sendtokbd(KBD_CMD_NOBELL);
		return;
	}
#endif KEYBS2
#ifdef S2FRAMEBUF
	/* Sun-2 frame buffer: flash the screen */
	copyvid = *VIDEOCTL_BASE;	/* Read current video controls */
	copyvid.vc_video_en = 0;	/* Disable video output */
	*VIDEOCTL_BASE = copyvid;
	while (i--);			/* Wait a moment... */
	copyvid.vc_video_en = 1;	/* Enable video output */
	*VIDEOCTL_BASE = copyvid;
#else  S2FRAMEBUF
	/* Sun-1 frame buffer: flash the screen */
        GXcontrol = 0;
	while (i--);
        GXcontrol = GXvideoEnable;
#endif S2FRAMEBUF
}

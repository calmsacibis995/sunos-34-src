/*
 * @(#)keypress.c 2.15 84/02/08 Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * keypress.c
 *
 * This routine is called each time a keypress is received.  It should
 * be as fast and safe as possible, since it runs at high interrupt levels.
 * It should just stash the character somewhere and check for aborts.
 * All other processing is done in "getkey".
 */

#include "../h/sunmon.h"
#include "../h/globram.h"
#include "../h/keyboard.h"
#include "../h/asyncbuf.h"
#include "../h/s2addrs.h"
#include "../h/s2misc.h"

#define keybuf		gp->g_keybuf
#define keystate	gp->g_keystate
#define keybid		gp->g_keybid


/*
 * A keypress was received (from a parallel or serial keyboard).
 * Process it, and return zero for normalcy or nonzero to abort.
 */
int
keypress(key)
	register unsigned char key;
{

	switch (keystate) {

	normalstate:
		keystate = NORMAL;
	case NORMAL:
		if (key == ABORTKEY1) {
			keystate = ABORT1; break;
#ifdef KEYBVT
		} else if (key == IDLEKEY) {
			keystate = IDLE1;
#endif KEYBVT
		}

		bput (keybuf, key);
		break;

#ifdef KEYBVT
	case IDLE1:
		keybid = key;
		keystate = IDLE2;
		break;

	case IDLE2:
		if (key == IDLEKEY) keystate = IDLE1;
		else goto normalstate;
		break;
#endif KEYBVT

	case ABORT1:
		if (key == ABORTKEY2) {
			keystate = NORMAL;
			bputclr(keybuf);  /* Clear typeahead */
			abortfix(); /* Let our other half know that
				     these keys really did go down */
			/* Since we know the keyboard is here, and
			 * the gal is typing on it now, we oughta
			 * select it as input ('cuz InSource might
			 * be screwed up).  We
			 * make a somewhat riskier assumption that the
			 * keyboard is near the screen and she'll want
			 * her output on it.  (If there's no screen,
			 * we just punt output.  What is she doing
			 * with a keyboard and no screen anyway?)
			 */
			gp->g_insource = INKEYB;  /* Take keyb inp */
			if (gp->g_fbthere != 0)
				gp->g_outsink = OUTSCREEN;
			/* Break out to the monitor */
			return 1;
		} else {
			bput (keybuf, ABORTKEY1);
			goto normalstate;
		}

	case STARTUP:
#ifndef KEYBS2
		if (key == IDLEKEY) goto normalstate;
		break;
#endif KEYBS2
#ifdef KEYBS2
		if (key == RESETKEY) keystate = STARTUP2;
		if (key == ERRORKEY) keystate = STARTUPERR;
		break;

	case STARTUP2:
		keybid = key;
		bput (keybuf, RESETKEY);	/* Tell upper half */
		keystate = NORMAL;
		break;

	case STARTUPERR:
		bput (keybuf, ERRORKEY);	/* Tell upper half */
		/* We ignore the error indicator byte */
		keystate = NORMAL;
		break;
#endif KEYBS2

	}
	return 0;		/* After normalcy, return 0. */
}

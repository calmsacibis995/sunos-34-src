/*
 * @(#)getkey.c 2.13 84/02/08 Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * getkey.c
 *
 * Keyboard decoding routine for Sun Monitor
 *
 * This module decodes keyboard up/down codes put in a buffer by
 * "keypress", turns them into ASCII, and passes them on to the rest
 * of the world, one at a time.
 *
 * There are currently two options.  You can request up/down codes,
 * in which case we just leave the driving to you; or you can
 * request ASCII codes, in which case we deal with remembering
 * case shifts, Repeat keys, Ascii translations, etc, and you just
 * see the kind of bytes that might come in from a terminal.
 *
 * We default to providing ASCII.
 */


/*
 * #define KEYBARF invokes some consistency checks on the data received,
 * doing printf's if anything is wrong.
 */

#include "../h/s2addrs.h"
#include "../h/keyboard.h"
#include "../h/asyncbuf.h"
#include "../h/globram.h"

extern struct keyboard	*keytables[];
extern char *keystringtab[];

/* 
 * Getkey Initialization
 *
 * If KEYBVT (keyboard sends ID byte) is set, we take the byte as a
 * parameter, and use it to determine whether or not the CAPS LOCK
 * key is down.  If not, we assume all keys are up.
 */
#ifdef KEYBVT
initgetkey(idcode)
	unsigned char idcode;
#else  KEYBVT
initgetkey()
#endif KEYBVT
{
#ifdef KEYBARF
	unsigned char *keyptr = &gp->keyswitch[0];

	while (keyptr < &gp->keyswitch[128]) 
		*keyptr++ = RELEASED;
#endif KEYBARF

	gp->g_shiftmask = 0;
#ifdef KEYBVT
	if (idcode & 0x40) gp->g_shiftmask |= CAPSMASK;
#endif KEYBVT
	gp->g_translation = TR_ASCII;
	gp->g_keyrtick = 1000/30;	/* 30 cps repeat rate */
	gp->g_keyrinit = 500;		/* Wait .5 sec before repeating */
	gp->g_keyrkey = IDLEKEY;	/* Nothing happening now */
}


/*
 * This routine fixes up our tables after an Abort, so we won't
 * think we detected spurious ups or downs.  The refresh routine
 * has seen these two keys go down but has not told us about them.
 * We, however, will see them go up, and this lets us know that
 * their upgoings are OK.
 *
 * This routine is called from the refresh routine or the start
 * of the monitor code for an Abort.  If it fails, Aborts won't work.
 */
abortfix()
{

#ifdef KEYBARF
	gp->keyswitch[ABORTKEY1] = PRESSED;
	gp->keyswitch[ABORTKEY2] = PRESSED;
#endif KEYBARF
	/* Tell mainline that keyboard is idle, (even tho it isn't)
	   to avoid its thinking that an autorepeat should occur */
	bput (gp->g_keybuf, IDLEKEY);
}

/*
 * getkey()
 *
 * Returns a key code (if up/down codes being returned),
 * 	a byte of ASCII (if that's requested)
 * 	NOKEY (if no key has been hit).
 */
int getkey ()
{
	register unsigned char keycode, key, entry;
	unsigned char notrepeating;

    while (1) {
	/* Loop til a value-returning key is pressed or buffer is empty */

	if (!bgetp(gp->g_keybuf)) {
		/* No keystrokes.  See if autorepeat in action. */
		if (gp->g_keyrkey != IDLEKEY) {
			/* Yes.  See if it's time to tick yet. */
			if (gp->g_nmiclock >= gp->g_keyrtime) {
				gp->g_keyrtime = gp->g_keyrtick + gp->g_nmiclock;
				keycode = gp->g_keyrkey;
				notrepeating = 0;  /* We are repeating */
				goto interpretkeycode;
			}
		}
		/* Either no autorepeat or no tick yet. */
		return (NOKEY);
	}

	bget(gp->g_keybuf, keycode);
	notrepeating = 1;	/* We are not repeating */

interpretkeycode:

	key = KEYOF(keycode);

#ifdef KEYBARF
	if ((gp->keyswitch)[key] == STATEOF(keycode) && keycode != IDLEKEY) {
		printf ("Barf!  Key %d %s twice.\n",
			KEYOF(keycode), (STATEOF(keycode) == PRESSED)
			? "Pressed" : "Released" );
	}

	(gp->keyswitch)[key] = STATEOF(keycode);
#endif KEYBARF

	if (gp->g_translation == TR_NONE) {
		return (keycode);
	}
	/* The only other value currently supported is TR_ASCII. */

	/* Translate key number and current shifts to an action byte */
	if (gp->g_shiftmask & SHIFTMASK)
		entry = keytables[0]->k_shifted->keymap[key];
	else
		entry = keytables[0]->k_normal ->keymap[key];

	if (notrepeating) {
		register unsigned char enF0;

		enF0 = entry & 0xF0;
		if ((STATEOF(keycode) == PRESSED) &&
/* We might need to add the RESET/IDLE/ERROR entries here.  FIXME? */
		    (NOSCROLL  != entry) &&
		    (SHIFTKEYS != enF0) &&
		    (BUCKYBITS != enF0) ) {
			gp->g_keyrkey = keycode;
			gp->g_keyrtime = gp->g_nmiclock + gp->g_keyrinit;
		} else {
			if (key == KEYOF(gp->g_keyrkey))
				gp->g_keyrkey = IDLEKEY;
		}
	}

	/* If key is going up and is not a reset or shift key, ignore it. */
	if (STATEOF(keycode) != PRESSED) {
#ifdef KEYBS2
		if (keycode == RESETKEY)	/* Reset has key-up bit on */
			entry = RESET;
		else
#endif KEYBS2
		     if ((entry&0xF0) != SHIFTKEYS)
			entry = NOP;
	}

	switch (entry >> 4) {

	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		/* Map normal ascii depending on ctrl, capslock */
		if (gp->g_shiftmask & CTRLMASK) entry &= 0x1F;
		if ((gp->g_shiftmask & CAPSMASK) &&
			(entry >= 'a' && entry <= 'z'))
				entry += 'A' - 'a';
		return entry;

	case SHIFTKEYS >> 4:
		gp->g_shiftmask ^= 1 << (entry & 0x0F);
		break;

	case FUNNY >> 4:
		switch (entry) {
		case NOP:
			break;

#ifdef KEYBARF
		case OOPS:
			printf ("Undefined key 0x%x (%d)\n",
				keycode, key);
			break;

		case HOLE:
			printf ("Nonexistent key!  0x%x (%d)\n",
				keycode, key);
			break;
#endif KEYBARF

		case NOSCROLL:
			if (gp->g_shiftmask & CTLSMASK)	goto sendcq;
			else				goto sendcs;

		case CTRLS:
		sendcs:
			gp->g_shiftmask |= CTLSMASK;
			return (('S'-0x40) /* | gp->g_buckybits */);

		case CTRLQ:
		sendcq:
			gp->g_shiftmask &= ~CTLSMASK;
			return (('Q'-0x40) /* | gp->g_buckybits */);

		case IDLE:
#ifdef KEYBVT
			/* Minor hack to prevent keyboards unplugged
			 * in caps lock from retaining their capslock
			 * state when replugged.  This should be
			 * solved by using the capslock info in the 
			 * keyboard id byte, but that's too hard for today.
			 * FIXME by doing it right in newkeyboard().
			 */
			if (NOTPRESENT == keycode) gp->g_shiftmask = 0;
#endif KEYBVT
#ifdef KEYBARF
			{
			register char *keyptr = &((gp->keyswitch)[0]);
			register char *keyend = &((gp->keyswitch)[128]);

			for ( ; keyptr < keyend ; keyptr++) {
				if ( *keyptr == PRESSED )
					printf ("%d pressed at idle.\n",
					  keyptr - &gp->keyswitch[0]);
					*keyptr = RELEASED;
			}
			}
#endif KEYBARF
			/* Fall thru into RESET code */

		case RESET:
		gotreset:
			gp->g_shiftmask &= keytables[0]->k_idleshifts;
			gp->g_keyrkey = IDLEKEY;	/* Don't repeat */
			break;

#ifdef KEYBS2
		case ERROR:
			printf("Keyboard error detected\n");
			goto gotreset;
#endif KEYBS2

/* Remember when adding new entries that, if they should NOT auto-repeat,
they should be put into the IF statement just above this switch block.
*/
		default:
			goto badentry;
		}
		break;

	/*
	 * Remember when adding new entries that, if they should NOT
	 * auto-repeat, they should be put into the IF statement just above
	 * this switch block.
	 */
	default:
	badentry:
#ifdef KEYBARF
printf ("Bad keymap entry %x for key 0x%x (%d) with shiftmask %x\n",
entry, keycode, key, gp->g_shiftmask);
#endif KEYBARF
		break;
	}
    }
}

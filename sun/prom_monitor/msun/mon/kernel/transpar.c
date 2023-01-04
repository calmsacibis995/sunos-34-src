/*
 * @(#)transpar.c 1.9 83/09/19 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * transparent.c
 *
 * Transparent-mode module for Sun ROM monitor
 */

#include "../h/asmdef.h"
#include "../h/asyncbuf.h"
#include "../h/globram.h"
#include "../h/m68vectors.h"
#include "../h/reentrant.h"
#include "../h/sunmon.h"
#include "../h/suntimer.h"
#include "../h/s2addrs.h"
#include "../h/zsreg.h"

/*
 * transparent mode - cross connect current input and output to a UART (A or B).
 *
 * Since it is quite possible that current I/O is a UART and that 
 * the two UARTs are running
 * at different speeds, there is a potential flow control
 * problem here.  We leave this up to the user to solve;
 * this routine runs at the speed of the slower side.
 *
 * Note that the word "escape" as used in this module doesn't mean \033.
 * Rather, it means the "transparent mode escape character", which is used
 * to escape from transparent mode.  It is stored in gp->g_transpend, and
 * is set by the "x" command.
 */
transparent(selector)
	int selector;
{
	register unsigned char c;
	register int len;
	register unsigned char *cp;
	register int escpending = 0;	/* true if we just saw an escape */
	register int keybctls = 0;	/* True if last char keyed was ^S */
	register struct zscc_device *uart;	/* UART register address */
	long l5pointer = *EVEC_LEVEL5;   /* Save previous values */
	long l6pointer = *EVEC_LEVEL6;
	int transpint(), transpig();

	/* We must run interrupt-driven here because screen output is
	 * too slow (when scrolling) to keep up with 9600 or 19200 baud.
	 * We must save level 5 and 6 vectors, drop our prio to 4,
 	 * and field interrupts for awhile.  Then restore it all later.
	 *
	 * It turns out that we also need ^S/^Q (or some other) protocol
	 * to run faster than 2400 baud and scroll.  (We can run 9600
	 * if we don't scroll, but moving that megabit is just too much.)
	 *
	 * The above figures undoubtedly need modifying now that we can
	 * scroll more than one line at once.  Scrolling 2 lines at a time
	 * oughta be twice as fast, etc.  Thus 4 at a time should give full
	 * 9600 baud thruput without ^S.
	 */
	excvmake (EVEC_LEVEL5, transpint);
	excvmake (EVEC_LEVEL6, transpig);

	gp->g_transpaddr = uart = &SERIAL0_BASE[2-selector];

	uart->zscc_control = 1;		/* Select register 1 */
	uart->zscc_control = ZSWR1_RIE;	/* Enable receiver ints */
	uart->zscc_control = 9;		/* Select master enable reg */
	uart->zscc_control = ZSWR9_MASTER_IE;	/* Enable ints */

	initbuf (gp->g_transpbuf, TRANSPBUFSIZE);

	/* Now that we're set up, allow ints on levels 5, 6, & 7 */
	asm("andw #0x2400,sr");

	while (1) {
	    /* Has host typed a character(s) ? */
	    if (gp->g_outsink == OUTSCREEN) {
		bgets (gp->g_transpbuf, cp, len);
		if (len) {  /* Avoid cursorflash if len=0 */
			fwritestr (cp, len);
			baccs (gp->g_transpbuf, len);
		}
	    } else
		    if (bgetp (gp->g_transpbuf)) {
			bget (gp->g_transpbuf, c);
			while (mayput(c)) ;
		    }

	    /* Has user typed a character? */
	    if ( (len = mayget() ) >= 0) {
		c = (unsigned char) len;
		/* Now determine whether we're escaping from transp mode. */
		if (gp->g_insource == INKEYB) {
			/* Keyboards escape with Setup-E (or Setup-e) */
			if ((len & ~('a'^'A')) == (SYSTEMMASK|'E')) break;
		} else {
			/* UARTs escape with (EndTransp)C; ux sets EndTransp.*/
			if (escpending)
			    if ((c&UPCASE) == 'C') {
				break;	/* esc-c: end transparent mode */
			    }
			    else escpending = 0; /* clear flag, send 2nd char */
			else if (c == gp->g_transpend) {
				escpending++;
				continue;
			}
		}
		if (c == TRANSPHALTCHAR) keybctls = 1;
		else if (c == TRANSPHALTENDCHAR) keybctls = 0;

SendChar:
		while(!(uart->zscc_control & ZSRR0_TX_READY)) ;
		uart->zscc_data = c;

		/* If we sent ^S due to buffer full, sending that char might
		   have broken us out of it (as far as the host is concerned).
		   Assume talking to Unix, where any char breaks ^S.
		   If this isn't Unix, we'll just send extra ^Ses. */
		if ((gp->g_transpstate > 0) /* We sent ^S */ &&
		    (c != TRANSPHALTCHAR) ) gp->g_transpstate = -1; 
	    } else {
		/* User didn't type anything, check how WE are doing */
		if (gp->g_transpstate > 0) {
			/* We sent a ^S, is is time to ^Q yet? */
			bsize(gp->g_transpbuf, len);
			if (len < TRANSPHALTMIN) {
			    if (keybctls)
				/* User typed a ^S.  Leave it that way */
				gp->g_transpstate = -1;  /* Just turn flag off */
				/* For now ^S and ^Q are only her business */
			    else {
				c = TRANSPHALTENDCHAR;
				goto SendChar;
			    }
			}
		}
	    }
	}

	asm ("orw #0x0700,sr");		/* Restore level 7 */
	/* We'd restore the chip status of the UART, but the damn thing
	   can't be read back! -- might not be true for ZSCC, FIXME */
	*EVEC_LEVEL5 = l5pointer;
	*EVEC_LEVEL6 = l6pointer;
}


reentrant(transpint)
{
	register struct zscc_device *uart = gp->g_transpaddr;
	register char c;
	register short len;
	
	if (uart->zscc_control & ZSRR0_RX_READY) {
		c = uart->zscc_data;
		bput (gp->g_transpbuf, c&NOPARITY);
		if (gp->g_transpstate < 0) {
			/* We want ^S/^Q support.  Check buffer len */
			/* Note that we won't send a second ^S until we have
			   sent a non-^S, even if we overflow. */
			bsize (gp->g_transpbuf, len);
			if (len >= TRANSPHALTMAX) {
				/* We kill time here which could be used */
				while(!(uart->zscc_control & ZSRR0_TX_READY)) ;
				uart->zscc_data = TRANSPHALTCHAR;
				gp->g_transpstate = 1;
			}
		}
	} else {
		/* If we get any but RX ints on our line, reset other line. */
		/* FIXME: C compiler doesn't like the folowing.  So kludge:
		uart = (struct zscc_device *) (int(uart) ^ 
			((int)(SERIAL0_BASE) ^ (int)(SERIAL0_BASE+1) ));
		FIXME: */
		if (uart == SERIAL0_BASE) uart = SERIAL0_BASE+1;
		else			  uart = SERIAL0_BASE;
		uart->zscc_control = 1;
		uart->zscc_control = 0;
	}
}

reentrant(transpig)
{
	/* If we ever get an interrupt from the clock, we just turn the
	   interrupt signal off.  It should be running in TC Toggled
	   mode, so it'll come back on later.  Whoever's listening loses
	   a few heartbeats.  Tough.
	*/
	TMRLoadCmd (TCClrOutput (TIMMisc));
	TMRLoadCmd (TCClrOutput (TIMSteal));
	TMRLoadCmd (TCClrOutput (TIMUser));
	TMRLoadCmd (TCClrOutput (TIMSuper));
}

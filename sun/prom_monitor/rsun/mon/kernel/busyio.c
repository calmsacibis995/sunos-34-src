/*
 * @(#)busyio.c 2.10 84/08/10 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * busyio.c
 *
 * busywait I/O module for Sun ROM monitor
 */

#include "../h/sasun.h"
#include "../h/zsreg.h"
#include "../h/sunmon.h"
#include "../h/globram.h"


putchar(x)
	register unsigned char x;
{

	if (x == '\n') putchar('\r');

	while (mayput (x)) ;

}


/*
 *	"Maybe put" routine -- puts a character if possible, returns
 *	zero if it did and -1 if it didn't (because Uart wasn't ready).
 */
int
mayput(x)
	unsigned char x;
{

	if (OUTSCREEN == gp->g_outsink) {
		fwritechar (x);
		return 0;
	}

	/* 6 PCLK cycles (1200ns) must elapse between zscc accesses. */
	if (gp->g_outzscc[2-gp->g_outsink].zscc_control & ZSRR0_TX_READY) {
		DELAY(10);
		gp->g_outzscc[2-gp->g_outsink].zscc_data = x;
		return 0;
	}

	return -1;
}


/*
 * Get, and echo if desired, a characters.  Wait until one arrives.
 */
unsigned char
getchar()
{
	register int c;

	do  c = mayget(); while (c<0);

	if (gp->g_echo)
	    putchar( (unsigned char)c);

	return (c);
}


/*
 * Maybe get a character.  Return it if one is there, else return -1.
 */
int
mayget()
{

#ifdef KEYBOARD
	if (INKEYB == gp->g_insource)
		return (getkey());  /* Just pick up a keyboard char & quit. */
#endif KEYBOARD

	/* 6 PCLK cycles (1200ns) must elapse between zscc accesses. */
	if (!(gp->g_inzscc[2-gp->g_insource].zscc_control & ZSRR0_RX_READY))
		return (-1);	/* no char pending */

	DELAY(10);
	return gp->g_inzscc[2-gp->g_insource].zscc_data & NOPARITY;
}

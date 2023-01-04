#ifndef lint
static	char sccsid[] = "@(#)draino.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

/*
 * Code for various kinds of delays.  Most of this is nonportable and
 * requires various enhancements to the operating system, so it won't
 * work on all systems.  It is included in curses to provide a portable
 * interface, and so curses itself can use it for function keys.
 */

#include "curses.ext"

#define NAPINTERVAL 100
/*
 * Wait until the output has drained enough that it will only take
 * ms more milliseconds to drain completely.
 * Needs Berkeley TIOCOUTQ ioctl.  Returns ERR if impossible.
 */
int
draino(ms)
int ms;
{
#ifdef TIOCOUTQ
# define _DRAINO
	int ncneeded;	/* number of chars = that many ms */

	/* 10 bits/char, 1000 ms/sec, baudrate in bits/sec */
	ncneeded = baudrate() * ms / (10 * 1000);
	for (;;) {
		int rv;		/* ioctl return value */
		int ncthere = 0;/* number of chars actually in output queue */
		rv = ioctl(cur_term->Filedes, TIOCOUTQ, &ncthere);
#ifdef DEBUG
		if (outf) fprintf(outf, "draino: rv %d, ncneeded %d, ncthere %d\n",
			rv, ncneeded, ncthere);
#endif
		if (rv < 0)
			return ERR;	/* ioctl didn't work */
		if (ncthere <= ncneeded) {
			return OK;
		}
		napms(NAPINTERVAL);
	}
	/*NOTREACHED*/
#endif

#ifdef TCSETAW
# define _DRAINO
	/*
	 * USG simulation - waits until the entire queue is empty,
	 * then sets the state to what it already is (e.g. no-op).
	 * Unfortunately this only works if ms is zero.
	 */
	if (ms <= 0) {
		if (ioctl(cur_term->Filedes, TCSETAW, cur_term->Nttyb) == 0)
			return OK;
	}
	return ERR;
#endif

#ifndef _DRAINO
	/* No way to fake it, so we return failure. */
	return ERR;
#endif
}

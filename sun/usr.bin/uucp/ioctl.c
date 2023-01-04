/*	ioctl.c	1.1	86/09/25	*/
	/*  ioctl 3.1  10/26/79  11:32:27  */
#include "uucp.h"
#include <sgtty.h>


/*******
 *	ioctl(fn, com, ttbuf)	for machines without ioctl
 *	int fn, com;
 *	struct sgttyb *ttbuf;
 *
 *	return codes - same as stty and gtty
 */

ioctl(fn, com, ttbuf)
int fn, com;
struct sgttyb *ttbuf;
{
	struct sgttyb tb;

	switch (com) {
	case TIOCHPCL:
		gtty(fn, &tb);
		tb.sg_flags |= 1;
		return(stty(fn, &tb));
	case TIOCGETP:
		return(gtty(fn, ttbuf));
	case TIOCSETP:
		return(stty(fn, ttbuf));
	case TIOCEXCL:
	default:
		return(-1);
	}
}

#ifndef lint
static	char sccsid[] = "@(#)stty.c 1.1 86/09/24 SMI"; /* from UCB 4.2 83/07/04 */
#endif

/*
 * Writearound to old stty system call.
 */

#include <sgtty.h>

stty(fd, ap)
	struct sgttyb *ap;
{

	return(ioctl(fd, TIOCSETP, ap));
}

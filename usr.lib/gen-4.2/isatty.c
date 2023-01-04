#ifndef lint
static	char sccsid[] = "@(#)isatty.c 1.1 86/09/24 SMI"; /* from UCB 4.1 80/12/21 */
#endif

/*
 * Returns 1 iff file is a tty
 */

#include <sgtty.h>

isatty(f)
{
	struct sgttyb ttyb;
	int err;
	extern int errno;

	err = errno;
	if (ioctl(f, TIOCGETP, (char *)&ttyb) < 0) {
		errno = err;
		return(0);
	}
	return(1);
}

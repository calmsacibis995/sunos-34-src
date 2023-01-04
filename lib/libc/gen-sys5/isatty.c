#ifndef lint
static	char sccsid[] = "@(#)isatty.c 1.1 86/09/24 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
/*
 * Returns 1 iff file is a tty
 */
#include <sys/termio.h>

extern int ioctl();

int
isatty(f)
int	f;
{
	struct termio tty;
	int err;
	extern int errno;

	err = errno;
	if(ioctl(f, TCGETA, &tty) < 0) {
		errno = err;
		return(0);
	}
	return(1);
}

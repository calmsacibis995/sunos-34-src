#ifndef lint
static	char sccsid[] = "@(#)utime.c 1.1 86/09/24 SMI"; /* from UCB 4.2 83/05/31 */
#endif

#include <sys/time.h>
/*
 * Backwards compatible utime.
 */
utime(name, otv)
	char *name;
	int otv[];
{
	struct timeval tv[2];

	tv[0].tv_sec = otv[0]; tv[0].tv_usec = 0;
	tv[1].tv_sec = otv[1]; tv[1].tv_usec = 0;
	return (utimes(name, tv));
}

#ifndef lint
static	char sccsid[] = "@(#)utime.c 1.1 86/09/24 SMI"; /* from UCB 4.2 83/05/31 */
#endif

#include <sys/time.h>

extern long time();

/*
 * Backwards compatible utime.
 *
 * The System V system call allows any user with write permission
 * on a file to set the accessed and modified times to the current
 * time; they specify this by passing a null pointer to "utime".
 * This is done to simulate reading one byte from a file and
 * overwriting that byte with itself, which is the technique used
 * by older versions of the "touch" command.  The advantage of this
 * hack in the system call is that it works correctly even if the file
 * is zero-length.
 *
 * This could be done by changing "utimes" in 4.2BSD, except that the
 * NFS currently doesn't support it.  This is the best we can do; it 
 * still requires you to be the owner of the file or the super-user.
 */
utime(name, otv)
	char *name;
	int otv[];
{
	struct timeval tv[2];

	if (otv == 0) {
		tv[0].tv_sec = time((long *)0);
		tv[0].tv_usec = 0;
		tv[1].tv_sec = tv[0].tv_sec;
		tv[1].tv_usec = 0;
	} else {
		tv[0].tv_sec = otv[0];
		tv[0].tv_usec = 0;
		tv[1].tv_sec = otv[1];
		tv[1].tv_usec = 0;
	}
	return (utimes(name, tv));
}

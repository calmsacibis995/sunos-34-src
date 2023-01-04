#ifndef lint
static	char sccsid[] = "@(#)abort.c 1.1 86/09/24 SMI"; /* from S5R2 1.4 */
#endif

/*LINTLIBRARY*/
/*
 *	abort() - terminate current process with dump via SIGIOT
 */

#include <signal.h>

extern int kill(), getpid();
extern void _cleanup();
static pass = 0;		/* counts how many times abort has been called*/

int
abort()
{
	/* increment first to avoid any hassle with interupts */
	if (++pass == 1)  {
		_cleanup();
	}
	return(kill(getpid(), SIGIOT));
}

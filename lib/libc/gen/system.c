#ifndef lint
static	char sccsid[] = "@(#)system.c 1.1 86/09/24 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
#include <signal.h>

extern int vfork(), execl(), wait();

int
system(s)
char	*s;
{
	int	status, pid, w;
	register int (*istat)(), (*qstat)();

	if((pid = vfork()) == 0) {
		(void) execl("/bin/sh", "sh", "-c", s, (char *)0);
		_exit(127);
	}
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	while((w = wait(&status)) != pid && w != -1)
		;
	(void) signal(SIGINT, istat);
	(void) signal(SIGQUIT, qstat);
	return((w == -1)? w: status);
}

#ifndef lint
static	char sccsid[] = "@(#)times.c 1.1 86/09/24 SMI"; /* from UCB 4.2 83/06/02 */
#endif

#include <sys/time.h>
#include <sys/resource.h>

/*
 * Backwards compatible times.
 */
struct tms {
	int	tms_utime;		/* user time */
	int	tms_stime;		/* system time */
	int	tms_cutime;		/* user time, children */
	int	tms_cstime;		/* system time, children */
};

#include "epoch.h"

long
times(tmsp)
	register struct tms *tmsp;
{
	struct rusage ru;
	struct timeval now;

	if (getrusage(RUSAGE_SELF, &ru) < 0)
		return (-1);
	tmsp->tms_utime = scale60(&ru.ru_utime);
	tmsp->tms_stime = scale60(&ru.ru_stime);
	if (getrusage(RUSAGE_CHILDREN, &ru) < 0)
		return (-1);
	tmsp->tms_cutime = scale60(&ru.ru_utime);
	tmsp->tms_cstime = scale60(&ru.ru_stime);
	if (gettimeofday(&now, (struct timezone *)0) < 0)
		return (-1);
	now.tv_sec -= epoch;
	return (scale60(&now));
}

static
scale60(tvp)
	register struct timeval *tvp;
{

	return (tvp->tv_sec * 60 + tvp->tv_usec / 16667);
}

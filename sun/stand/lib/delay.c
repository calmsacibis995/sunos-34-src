#include <sys/types.h>
/*
 *	these little gems delay a given amount of time in millisecs
 *	fuzzdelay even takes refresh into account by using a
 *	fudge factor in the timing loop
 *	fuzzdelay assumes that the sun2 "heartbeat" is running
 *	in that calculation
 */
#define	ONEMSEC	(447)
#define FUZZ	(.9425723795)

static char	sccsid[] = "@(#)delay.c 1.1 9/25/86 Copyright Sun Micro";

delay(time)
register u_long	time;
{
	register u_long	timer = time*ONEMSEC;
	
	do {timer -= 1;} while (timer > 0);
}

fuzzdelay(time)
register u_long	time;
{
	register u_long	timer = (u_long)(FUZZ*((double)(time*ONEMSEC)));
	
	do {timer -= 1;} while (timer > 0);
}

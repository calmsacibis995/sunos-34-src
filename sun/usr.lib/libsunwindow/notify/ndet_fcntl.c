#ifndef	lint
static	char sccsid[] = "@(#)ndet_fcntl.c 1.4 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_fcntl.c - Notifier's version of fcntl.  Used to detect async
 *		   mode of fds managing.
 */

#include "ntfy.h"
#include "ndet.h"
#include <signal.h>
#include <fcntl.h>

extern int
fcntl(fd, cmd, arg)
	int fd, cmd, arg;
{
	register int bit = FD_BIT(fd);
	int res;

	/* If call fails then ignore transition */
	if ((res = notify_fcntl(fd, cmd, arg)) == -1)
		return(res);
	/*
	 * Update non-blocking read and async data ready flags if setting
	 * or querying.  Doing it on querying double checks information.
	 */
	if (cmd == F_SETFL || cmd == F_GETFL) {
		/* For F_GETFL, res contains flags */
		if (cmd == F_GETFL)
			arg = res;
		NTFY_BEGIN_CRITICAL;
		if (arg & FNDELAY)
			ndet_fndelay_mask |= bit;
		else
			ndet_fndelay_mask &= ~bit;
		if (arg & FASYNC)
			ndet_fasync_mask |= bit;
		else
			ndet_fasync_mask &= ~bit;
		/* Make sure that are catching async related signals now */
		if (ndet_fasync_mask) {
			ndet_enable_sig(SIGIO);
			ndet_enable_sig(SIGURG);
		}
		/*
		 * Setting NDET_FD_CHANGE will "fix up" signals being caught
		 * to be the minimum required next time around the
		 * notification loop.
		 */
		ndet_flags |= NDET_FD_CHANGE;
		NTFY_END_CRITICAL;
	}
	return(res);
}


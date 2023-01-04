#ifndef	lint
static	char sccsid[] = "@(#)ndet_wait.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_wait.c - Implement wait3 specific calls.
 */

#include "ntfy.h"
#include "ndet.h"
#include <errno.h>

extern	errno;

pkg_private int
ndet_check_pid(pid)
	int pid;
{
	if (kill(pid, 0)) {
		ntfy_set_errno((errno == ESRCH)? NOTIFY_SRCH: NOTIFY_INVAL);
		return(-1);
	}
	return(0);
}


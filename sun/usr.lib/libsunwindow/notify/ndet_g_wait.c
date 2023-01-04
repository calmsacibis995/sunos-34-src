#ifndef	lint
static	char sccsid[] = "@(#)ndet_g_wait.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_g_wait.c - Implement the notify_get_wait3_func interface.
 */

#include "ntfy.h"
#include "ndet.h"

extern Notify_func
notify_get_wait3_func(nclient, pid)
	Notify_client nclient;
	int pid;
{
	/* Check arguments */
	if (ndet_check_pid(pid))
		return(NOTIFY_FUNC_NULL);
	return(ndet_get_func(nclient, NTFY_WAIT3,
	    (NTFY_DATA)pid, NTFY_USE_DATA));
}


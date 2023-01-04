#ifndef	lint
static  char sccsid[] = "@(#)ndis_d_wait.c 1.4 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndis_d_wait.c - Default wait3 function that is a nop.
 */
#include "ntfy.h"
#include "ndis.h"
#include <signal.h>

/* ARGSUSED */
extern Notify_value
notify_default_wait3(client, pid, status, rusage)
	Notify_client client;
	int pid;
	union	wait *status;
	struct	rusage *rusage;
{
	return(NOTIFY_IGNORED);
}


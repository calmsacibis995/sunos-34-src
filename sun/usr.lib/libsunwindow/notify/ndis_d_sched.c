#ifndef	lint
static	char sccsid[] = "@(#)ndis_d_sched.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndis_d_sched.c - Default scheduler for the dispatcher.
 */

#include "ntfy.h"
#include "ndis.h"

pkg_private Notify_value
ndis_default_scheduler(n, nclients)
	int n;
	register Notify_client *nclients;
{
	register Notify_client nclient;
	register int i;

	for (i = 0; i < n;i++) {
		nclient = *(nclients+i);
		/* Notify client if haven't been done yet */
		if (nclient != NOTIFY_CLIENT_NULL) {
			/* notify_client detects errors from nclients */
			if (notify_client(nclient) != NOTIFY_OK)
				return(NOTIFY_UNEXPECTED);
			/*
			 * Null out client entry prevents it from being
			 * notified again.
			 */
			*(nclients+i) = NOTIFY_CLIENT_NULL;
		}
	}
	return(NOTIFY_DONE);
}


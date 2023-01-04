#ifndef	lint
static	char sccsid[] = "@(#)ndet_single.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_single.c - Implement notify_dispatch, i.e., do one notification cycle,
 *		   if not already in the middle of it.
 */

#include "ntfy.h"
#include "ndet.h"
#include "ndis.h"	/* For ndis_client == NTFY_CLIENT_NULL check */
#include <errno.h>

extern Notify_error
notify_dispatch()
{
	Notify_error return_code;

	/*
	 * Do nothing if in middle of notification loop or
	 * no other clients and no notifications pending.
	 */
	if (ndet_flags & NDET_STARTED)
		return (NOTIFY_INVAL);
	if ((ndet_clients == NTFY_CLIENT_NULL) &&
	      (ndis_clients == NTFY_CLIENT_NULL))
		return (NOTIFY_NO_CONDITION);
	/* Setup flag to break out of notify_start after single loop */
	ndet_flags |= NDET_NO_DELAY;
	/* Start notifier */
	return_code = notify_start();
	/* Reset break out flag  */
	ndet_flags &= ~NDET_NO_DELAY;
	return (return_code);
}


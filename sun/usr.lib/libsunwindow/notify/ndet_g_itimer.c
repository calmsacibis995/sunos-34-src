#ifndef	lint
static	char sccsid[] = "@(#)ndet_g_itimer.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_g_itimer.c - Implement the notify_get_itimer_func interface.
 */

#include "ntfy.h"
#include "ndet.h"

extern Notify_func
notify_get_itimer_func(nclient, which)
	Notify_client nclient;
	int which;
{
	NTFY_TYPE type;

	/* Check arguments */
	if (ndet_check_which(which, &type))
		return(NOTIFY_FUNC_NULL);
	return(ndet_get_func(nclient, type, NTFY_DATA_NULL, NTFY_IGNORE_DATA));
}


#ifndef	lint
static	char sccsid[] = "@(#)nint_i_itimer.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_i_itimer.c - Implement the notify_interpose_itimer_func interface.
 */

#include "ntfy.h"
#include "ndet.h"
#include "nint.h"

extern Notify_error
notify_interpose_itimer_func(nclient, func, which)
	Notify_client nclient;
	Notify_func func;
	int which;
{
	NTFY_TYPE type;

	/* Check arguments */
	if (ndet_check_which(which, &type))
		return(notify_errno);
	return(nint_interpose_func(nclient, func, type, NTFY_DATA_NULL,
	    NTFY_IGNORE_DATA));
}

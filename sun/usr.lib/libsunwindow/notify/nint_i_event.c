#ifndef	lint
static	char sccsid[] = "@(#)nint_i_event.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_i_event.c - Implement the notify_interpose_event_func interface.
 */

#include "ntfy.h"
#include "ndet.h"
#include "nint.h"

extern Notify_error
notify_interpose_event_func(nclient, func, when)
	Notify_client nclient;
	Notify_func func;
	Notify_event_type when;
{
	NTFY_TYPE type;

	/* Check arguments */
	if (ndet_check_when(when, &type))
		return(notify_errno);
	return(nint_interpose_func(nclient, func, type, NTFY_DATA_NULL,
	    NTFY_IGNORE_DATA));
}


#ifndef	lint
static	char sccsid[] = "@(#)nint_n_event.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_n_event.c - Implement the notify_next_event_func interface.
 */

#include "ntfy.h"
#include "ndet.h"
#include "nint.h"

extern Notify_value	
notify_next_event_func(nclient, event, arg, when)
	Notify_client nclient;
	Notify_event event;
	Notify_arg arg;
	Notify_event_type when;
{
	Notify_func func;
	NTFY_TYPE type;

	/* Check arguments */
	if (ndet_check_when(when, &type))
		return(NOTIFY_UNEXPECTED);
	if ((func = nint_next_callout(nclient, type)) == NOTIFY_FUNC_NULL)
		return(NOTIFY_UNEXPECTED);
	return(func(nclient, event, arg, when));
}


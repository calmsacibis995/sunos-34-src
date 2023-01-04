#ifndef	lint
static	char sccsid[] = "@(#)nint_n_death.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_n_death.c - Implement the notify_next_destroy_func interface.
 */

#include "ntfy.h"
#include "ndet.h"
#include "nint.h"

extern Notify_value	
notify_next_destroy_func(nclient, status)
	Notify_client nclient;
	Destroy_status status;
{
	Notify_func func;

	/* Check arguments */
	if (ndet_check_status(status))
		return(NOTIFY_UNEXPECTED);
	if ((func = nint_next_callout(nclient, NTFY_DESTROY)) ==
	    NOTIFY_FUNC_NULL)
		return(NOTIFY_UNEXPECTED);
	return(func(nclient, status));
}


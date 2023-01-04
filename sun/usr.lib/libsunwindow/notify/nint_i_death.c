#ifndef	lint
static	char sccsid[] = "@(#)nint_i_death.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_i_death.c - Implement the notify_interpose_destroy_func interface.
 */

#include "ntfy.h"
#include "ndet.h"
#include "nint.h"

extern Notify_error	
notify_interpose_destroy_func(nclient, func)
	Notify_client nclient;
	Notify_func func;
{
	return(nint_interpose_func(nclient, func, NTFY_DESTROY, NTFY_DATA_NULL,
	    NTFY_IGNORE_DATA));
}


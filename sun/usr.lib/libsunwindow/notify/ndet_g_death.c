#ifndef	lint
static	char sccsid[] = "@(#)ndet_g_death.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_g_destroy.c - Implement the notify_get_destroy_func interface.
 */

#include "ntfy.h"
#include "ndet.h"

extern Notify_func
notify_get_destroy_func(nclient)
	Notify_client nclient;
{
	return(ndet_get_func(nclient, NTFY_DESTROY, NTFY_DATA_NULL,
	    NTFY_IGNORE_DATA));
}


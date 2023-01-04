#ifndef	lint
static	char sccsid[] = "@(#)ndis_g_sched.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndis_g_sched.c - Implement the notify_get_sheduler_func.
 */

#include "ntfy.h"
#include "ndis.h"

extern Notify_func
notify_get_scheduler_func()
{
	return(ndis_scheduler);
}


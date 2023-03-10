#ifndef	lint
static	char sccsid[] = "@(#)ndis_s_sched.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndis_s_sched.c - Implement the notify_set_sheduler_func.
 */

#include "ntfy.h"
#include "ndis.h"

extern Notify_func
notify_set_scheduler_func(scheduler_func)
	Notify_func scheduler_func;
{
	register Notify_func old_func;

	NTFY_BEGIN_CRITICAL;
	old_func = ndis_scheduler;
	ndis_scheduler = scheduler_func;
	if (ndis_scheduler == NOTIFY_FUNC_NULL)
		ndis_scheduler = ndis_default_scheduler;
	NTFY_END_CRITICAL;
	return(old_func);
}


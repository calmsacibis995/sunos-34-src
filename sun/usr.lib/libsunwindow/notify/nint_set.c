#ifndef	lint
static	char sccsid[] = "@(#)nint_set.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_set.c - Implement the nint_set_func private interface.
 */

#include "ntfy.h"
#include "ndet.h"
#include "nint.h"

pkg_private Notify_func
nint_set_func(cond, new_func)
	register NTFY_CONDITION *cond;
	Notify_func new_func;
{
	Notify_func old_func;

	if (cond->func_count > 1) {
		old_func = cond->callout.functions[cond->func_count-1];
		cond->callout.functions[cond->func_count-1] = new_func;
	} else {
		old_func = cond->callout.function;
		cond->callout.function = new_func;
		cond->func_count = 1;
	}
	return(old_func);
}


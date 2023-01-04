#ifndef	lint
static	char sccsid[] = "@(#)nint_get.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_get.c - Implement the nint_get_func private interface.
 */

#include "ntfy.h"
#include "ndet.h"
#include "nint.h"

pkg_private Notify_func
nint_get_func(cond)
	register NTFY_CONDITION *cond;
{
	Notify_func func;

	if (cond->func_count > 1)
		func = cond->callout.functions[cond->func_count-1];
	else
		func = cond->callout.function;
	return(func);
}


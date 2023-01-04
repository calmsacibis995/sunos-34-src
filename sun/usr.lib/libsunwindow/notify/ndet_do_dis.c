#ifndef	lint
static	char sccsid[] = "@(#)ndet_do_dis.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_do_dis.c - In order for "background" dispatching to be done,
 *		   notify_do_dispatch must be called.  Background dispatching
 *		   is that dispatching which is done when a read or select is
 *		   called before calling notify_start.
 */

#include "ntfy.h"
#include "ndet.h"

extern Notify_error
notify_do_dispatch()
{
	ndet_flags |= NDET_DISPATCH;
	return (NOTIFY_OK);
}


#ifndef	lint
static	char sccsid[] = "@(#)ndet_event.c 1.4 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_event.c - Implement event specific calls that are shared among
 * NTFY_SAFE_EVENT and NTFY_IMMEDIATE_EVENT.
 */

#include "ntfy.h"
#include "ndet.h"

pkg_private int
ndet_check_when(when, type_ptr)
	Notify_event_type when;
	NTFY_TYPE *type_ptr;
{
	NTFY_TYPE type;

	switch (when) {
	case NOTIFY_SAFE: type = NTFY_SAFE_EVENT; break;
	case NOTIFY_IMMEDIATE: type = NTFY_IMMEDIATE_EVENT; break;
	default: ntfy_set_errno(NOTIFY_INVAL); return(-1);
	}
	if (type_ptr)
		*type_ptr = type;
	return(0);
}


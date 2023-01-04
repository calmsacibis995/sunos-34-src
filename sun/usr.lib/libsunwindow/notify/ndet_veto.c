#ifndef	lint
static	char sccsid[] = "@(#)ndet_veto.c 1.5 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_veto.c - Implementation of notify_veto_destroy.
 */

#include "ntfy.h"
#include "ndet.h"

/* ARGSUSED */
extern Notify_error
notify_veto_destroy(nclient)
	Notify_client nclient;
{
	NTFY_BEGIN_CRITICAL;
	ndet_flags |= NDET_VETOED;
	NTFY_END_CRITICAL;
	return(NOTIFY_OK);
}


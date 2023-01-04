#ifndef	lint
static	char sccsid[] = "@(#)ndet_die.c 1.4 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_die.c - Notify_die implementation.
 */

#include "ntfy.h"
#include "ndet.h"
#include "ndis.h"

extern Notify_error
notify_die(status)
	Destroy_status status;
{
	NTFY_ENUM ndet_immediate_destroy(), ndet_remove_all();
	NTFY_ENUM enum_code;
	Notify_error return_code;

	if (ndet_check_status(status))
		return(NOTIFY_INVAL);
	NTFY_BEGIN_CRITICAL;
	/* Call all destroy procs (go around entire dispatch mechanism) */
	enum_code = ntfy_paranoid_enum_conditions(ndet_clients,
	    ndet_immediate_destroy, (NTFY_ENUM_DATA)status);
	/* If checking then return result */
	return_code = NOTIFY_OK;
	if (status == DESTROY_CHECKING) {
		if (enum_code == NTFY_ENUM_TERM)
			return_code = NOTIFY_DESTROY_VETOED;
	} else
		/* else remove all clients */
		(void) ntfy_paranoid_enum_conditions(ndet_clients,
		    ndet_remove_all, NTFY_ENUM_DATA_NULL);
	NTFY_END_CRITICAL;
	return(return_code);
}

/*
 * Remove each client.
 */
/* ARGSUSED */
pkg_private NTFY_ENUM
ndet_remove_all(client, condition, context)
	NTFY_CLIENT *client;
	NTFY_CONDITION *condition;
	NTFY_ENUM_DATA context;
{
	(void) notify_remove(client->nclient);
	return(NTFY_ENUM_SKIP);
}


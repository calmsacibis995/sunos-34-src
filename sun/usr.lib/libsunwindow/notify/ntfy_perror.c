#ifndef	lint
static	char sccsid[] = "@(#)ntfy_perror.c 1.5 87/01/07 Copyr 1985 Sun Micro";
#endif
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ntfy_perror.c - Notify_perror implementation.
 */

#include <stdio.h>
#include "ntfy.h"

extern void
notify_perror(str)
	char *str;
{
	register char *msg;

	switch(notify_errno) {
	case NOTIFY_OK: msg = "Success"; break;
	case NOTIFY_UNKNOWN_CLIENT: msg = "Unknown client"; break;
	case NOTIFY_NO_CONDITION: msg = "No condition for client"; break;
	case NOTIFY_BAD_ITIMER: msg = "Unknown interval timer type"; break;
	case NOTIFY_BAD_SIGNAL: msg = "Bad signal number"; break;
	case NOTIFY_NOT_STARTED: msg = "Notifier not started"; break;
	case NOTIFY_DESTROY_VETOED: msg = "Destroy vetoed"; break;
	case NOTIFY_INTERNAL_ERROR: msg = "Notifier internal error"; break;
	case NOTIFY_SRCH: msg = "No such process"; break;
	case NOTIFY_BADF: msg = "Bad file number"; break;
	case NOTIFY_NOMEM: msg = "Not enough core"; break;
	case NOTIFY_INVAL: msg = "Invalid argument"; break;
	case Notify_func_LIMIT: msg = "Too many interposition functions"; break;
	default: msg = "Unknown notifier error";
	}
	(void) fprintf(stderr, "%s: %s\n", str, msg);
}


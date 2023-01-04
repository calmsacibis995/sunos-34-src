#ifndef	lint
static	char sccsid[] = "@(#)ndet_g_except.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_g_except.c - Implement notify_get_exception_func call.
 */

#include "ntfy.h"
#include "ndet.h"

extern Notify_func
notify_get_exception_func(nclient, fd)
	Notify_client nclient;
	int fd;
{
	return(ndet_get_fd_func(nclient, fd, NTFY_EXCEPTION));
}


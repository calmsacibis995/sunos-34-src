#ifndef	lint
static	char sccsid[] = "@(#)ndet_s_except.c 1.4 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_s_except.c - Implement notify_set_exception_func call.
 */

#include "ntfy.h"
#include "ndet.h"

extern Notify_func
notify_set_exception_func(nclient, func, fd)
	Notify_client nclient;
	Notify_func func;
	int fd;
{
	return(ndet_set_fd_func(nclient, func, fd, NTFY_EXCEPTION));
}


#ifndef	lint
static	char sccsid[] = "@(#)nint_r_except.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_r_except.c - Implement the notify_remove_exception_func interface.
 */

#include "ntfy.h"
#include "nint.h"

extern Notify_error
notify_remove_exception_func(nclient, func, fd)
	Notify_client nclient;
	Notify_func func;
	int fd;
{
	return(nint_remove_fd_func(nclient, func, NTFY_EXCEPTION, fd));
}


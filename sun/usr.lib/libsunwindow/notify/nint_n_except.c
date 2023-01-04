#ifndef	lint
static	char sccsid[] = "@(#)nint_n_except.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_n_except.c - Implement the notify_next_exception_func interface.
 */

#include "ntfy.h"
#include "ndet.h"
#include "nint.h"

extern Notify_value
notify_next_exception_func(nclient, fd)
	Notify_client nclient;
	int fd;
{
	return(nint_next_fd_func(nclient, NTFY_EXCEPTION, fd));
}


#ifndef	lint
static	char sccsid[] = "@(#)nint_r_fd.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_r_fd.c - Implement the nint_remove_fd_func private interface.
 */

#include "ntfy.h"
#include "ndet.h"
#include "nint.h"

pkg_private Notify_error	
nint_remove_fd_func(nclient, func, type, fd)
	Notify_client nclient;
	Notify_func func;
	NTFY_TYPE type;
	int fd;
{
	/* Check arguments */
	if (ndet_check_fd(fd))
		return(notify_errno);
	return(nint_remove_func(nclient, func, type, (NTFY_DATA)fd,
	    NTFY_USE_DATA));
}


#ifndef	lint
static	char sccsid[] = "@(#)ndet_fd.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_fd.c - Implement file descriptor specific calls that are shared among
 * NTFY_INPUT, NTFY_OUTPUT and NTFY_EXCEPTION.
 */

#include "ntfy.h"
#include "ndet.h"

static	int ndet_fd_table_size;	/* Number of descriptor slots available */

pkg_private int
ndet_check_fd(fd)
	int fd;
{
	if (ndet_fd_table_size == 0)
		ndet_fd_table_size = getdtablesize();
	if (fd < 0 || fd >= ndet_fd_table_size) {
		ntfy_set_errno(NOTIFY_BADF);
		return(-1);
	}
	return(0);
}


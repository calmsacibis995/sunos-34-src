#ifndef	lint
static	char sccsid[] = "@(#)sys_read.c 1.4 87/01/07 Copyr 1985 Sun Micro";
#endif
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Sys_read.c - Real system call to read.
 */

#include <syscall.h>
#include "ntfy.h"

pkg_private int
notify_read(fd, buf, nbytes)
	int fd;
	char *buf;
	int nbytes;
{
	return(syscall(SYS_read, fd, buf, nbytes));
}


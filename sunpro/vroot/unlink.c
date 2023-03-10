/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)unlink.c 1.1 87/01/08 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	unlink_thunk(path) char *path;
{
	vroot_result= unlink(path);
	return(vroot_result == 0);
}

int	unlink_vroot(path, vroot_path, vroot_vroot)
	char		*path;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	translate_with_thunk(path, unlink_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}

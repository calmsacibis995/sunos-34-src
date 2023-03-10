/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)execve.c 1.1 87/01/08 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <vroot.h>
#include "args.h"

static int	execve_thunk(path) char *path;
{
	execve(path, vroot_args.execve.argv, vroot_args.execve.environ);
	return(0);
}

int	execve_vroot(path, argv, environ, vroot_path, vroot_vroot)
	char		*path;
	char		**argv;
	char		**environ;
	pathpt		vroot_path;
	pathpt		vroot_vroot;
{
	vroot_args.execve.argv= argv;
	vroot_args.execve.environ= environ;
	translate_with_thunk(path, execve_thunk, vroot_path, vroot_vroot, rw_read);
	return(-1);
}

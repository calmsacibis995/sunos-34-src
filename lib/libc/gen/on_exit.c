
#ifndef lint
static	char sccsid[] = "@(#)on_exit.c 1.1 86/09/24 Copyr 1984 Sun Micro";
#endif
#include <sys/types.h>

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * on_exit -- name termination handler to be called by exit
 *            only NHANDLERS of them allowed.
 */

/*
 * structure to hold names and arguments of termination handlers
 */
struct tname {
    void      (*handler)();
    caddr_t	argument;
};

/* arbitrary constant (happens to agree with documentation) */
#define NHANDLERS	20

/* the list of handlers and their arguments */
extern int 	    _exit_nhandlers ;
extern struct tname _exit_tnames[NHANDLERS];

int
on_exit( procp, arg )
    void (*procp)( );
    caddr_t  arg;
{
    if (++_exit_nhandlers  >= NHANDLERS) return -1;
    _exit_tnames[_exit_nhandlers].handler = procp;
    _exit_tnames[_exit_nhandlers].argument = arg;
    return 0;
}


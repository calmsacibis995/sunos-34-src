#ifndef lint
static	char sccsid[] = "@(#)exit.c 1.1 86/09/24 Copyr 1984 Sun Micro";
#endif
#include <sys/types.h>

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * structure to hold names and arguments of termination handlers
 */
struct tname {
    void      (*handler)();
    caddr_t	argument;
};

extern void _cleanup(), _exit();

/* arbitrary constant (happens to agree with documentation) */
#define NHANDLERS	20

/* the list of handlers and their arguments */
int 	     _exit_nhandlers  = 0;
struct tname _exit_tnames[NHANDLERS] = {{ _cleanup, 0}};

/*
 * exit -- do termination processing, then evaporate process
 */

void
exit( code )
{
    /*
     * Note that we use the static "nhandlers" as a loop index here.
     * this is so any calls to on_exit by a termination handler
     * will cause the named handler to be called next. We actually
     * do not want to encourage this calling, but at least our actions
     * should be deterministic.
     */
    while (_exit_nhandlers >=0){
	_exit_nhandlers -= 1;
	(*_exit_tnames[_exit_nhandlers+1].handler)( code, _exit_tnames[_exit_nhandlers+1].argument );
    }
    _exit( code );
    /*NOTREACHED*/
}

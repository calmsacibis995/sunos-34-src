#ifndef lint
static	char sccsid[] = "@(#)sel_writable_data.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

#include "selection_impl.h"

/*
 *	sel_writable_data.c:	writable initialized data
 *				(must not go in text segment) 
 *
 */

struct timeval	    seln_std_timeout = {
    SELN_STD_TIMEOUT_SEC, SELN_STD_TIMEOUT_USEC
};

int		    seln_svc_program = SELN_SVC_PROGRAM;


#ifndef	lint
static	char sccsid[] = "@(#)nint_n_sig.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_n_sig.c - Implement the notify_next_signal_func interface.
 */

#include "ntfy.h"
#include "ndet.h"
#include "nint.h"

extern Notify_value	
notify_next_signal_func(nclient, signal, mode)
	Notify_client nclient;
	int signal;
	Notify_signal_mode mode;
{
	Notify_func func;
	NTFY_TYPE type;

	/* Check arguments */
	if (ndet_check_mode(mode, &type) ||ndet_check_sig(signal))
		return(NOTIFY_UNEXPECTED);
	if ((func = nint_next_callout(nclient, type)) == NOTIFY_FUNC_NULL)
		return(NOTIFY_UNEXPECTED);
	return(func(nclient, signal, mode));
}


#ifndef lint
static	char sccsid[] = "@(#)signal.c 1.1 86/09/24 SMI"; /* from UCB 5.1 85/06/05 */
#endif

/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Almost backwards compatible signal.
 */
#include <signal.h>

int (*
signal(s, a))()
	int s, (*a)();
{
	struct sigvec osv, sv;
	static int mask[NSIG];
	static int flags[NSIG];

	sv.sv_handler = a;
	sv.sv_mask = mask[s];
	sv.sv_flags = flags[s];
	if (sigvec(s, &sv, &osv) < 0)
		return (BADSIG);
	if (sv.sv_mask != osv.sv_mask || sv.sv_flags != osv.sv_flags) {
		mask[s] = sv.sv_mask = osv.sv_mask;
		flags[s] = sv.sv_flags = osv.sv_flags;
		if (sigvec(s, &sv, (struct sigvec *)0) < 0)
			return (BADSIG);
	}
	return (osv.sv_handler);
}

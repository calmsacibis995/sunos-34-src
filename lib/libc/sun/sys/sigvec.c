#ifndef lint
static	char sccsid[] = "@(#)sigvec.c 1.1 86/09/24 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include <signal.h>
#include <syscall.h>
#include <errno.h>

int	(*_sigfunc[NSIG])();
extern	int _sigtramp();
extern	int errno;

sigvec(sig, vec, ovec)
	int sig;
	register struct sigvec *vec, *ovec;
{
	struct sigvec avec;
	register int (*osig)(), (*nsig)();

	if (sig <= 0 || sig >= NSIG) {
		errno = EINVAL;
		return (-1);
	}
	osig = _sigfunc[sig];
	if (vec) {
		avec = *vec;
		vec = &avec;
		if ((nsig = vec->sv_handler) != SIG_DFL && nsig != SIG_IGN) {
			_sigfunc[sig] = nsig;
			vec->sv_handler = _sigtramp;
		}
	}
	if (syscall(SYS_sigvec, sig, vec, ovec) < 0) {
		_sigfunc[sig] = osig;
		return (-1);
	}
	if (ovec && ovec->sv_handler == _sigtramp)
		ovec->sv_handler = osig;
	return (0);
}

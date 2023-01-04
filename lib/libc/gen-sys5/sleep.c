#ifndef lint
static	char sccsid[] = "@(#)sleep.c 1.1 86/09/24 SMI";
#endif

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/time.h>
#include <signal.h>

#define	setvec(vec, a) \
	vec.sv_handler = a; vec.sv_mask = vec.sv_onstack = 0

unsigned
sleep(n)
	unsigned n;
{
	int sleepx(), omask;
	struct itimerval itv, oitv, zero;
	register struct itimerval *itp = &itv;
	struct timeval left_over;
	int alrm_flg;
	struct sigvec vec, ovec;

	if (n == 0)
		return(0);
	timerclear(&itp->it_interval);
	timerclear(&itp->it_value);
	if (setitimer(ITIMER_REAL, itp, &oitv) < 0)
		return(n);
	itp->it_value.tv_sec = n;
	alrm_flg = 0;
	timerclear(&left_over);
	if (timerisset(&oitv.it_value)) {
		if (timercmp(&oitv.it_value, &itp->it_value, >)) {
			oitv.it_value.tv_sec -= itp->it_value.tv_sec;
			++alrm_flg;
		} else {
			left_over.tv_sec = itp->it_value.tv_sec
			    - oitv.it_value.tv_sec;
			if (oitv.it_value.tv_usec != 0) {
				left_over.tv_sec--;
				left_over.tv_usec = 1000000
				    - oitv.it_value.tv_usec;
			}
			itp->it_value = oitv.it_value;
			timerclear(&oitv.it_value);
			--alrm_flg;
		}
	}
	if (alrm_flg >= 0) {
		setvec(vec, sleepx);
		(void) sigvec(SIGALRM, &vec, &ovec);
	}
	omask = sigblock(sigmask(SIGALRM));
	(void) setitimer(ITIMER_REAL, itp, (struct itimerval *)0);
	sigpause(omask &~ sigmask(SIGALRM));
	timerclear(&zero.it_value);
	timerclear(&zero.it_interval);
	(void) setitimer(ITIMER_REAL, &zero, itp);
	if (alrm_flg >= 0)
		(void) sigvec(SIGALRM, &ovec, (struct sigvec *)0);
	(void) sigsetmask(omask);
	if (alrm_flg > 0 || (alrm_flg < 0 && timerisset(&itp->it_value))) {
		/*
		 * Previous alarm was in the future of the sleep end,
		 * so we add the previous alarm (which is currently
		 * stored as a time delta from the current alarm)
		 * to the time till the sleep end (which may be
		 * zero if the sleep end has occurred) and set the
		 * alarm to that value.  We also restore the signal
		 * handler if we changed it.
		 */
		itp->it_value.tv_usec += oitv.it_value.tv_usec;
		itp->it_value.tv_sec += oitv.it_value.tv_sec;
		if (itp->it_value.tv_usec >= 1000000) {
			itp->it_value.tv_usec -= 1000000;
			itp->it_value.tv_sec++;
		}
		(void) setitimer(ITIMER_REAL, itp, (struct itimerval *)0);
	}
	left_over.tv_sec += itp->it_value.tv_sec;
	left_over.tv_usec += itp->it_value.tv_usec;
	if (left_over.tv_usec >= 1000000) {
		left_over.tv_sec++;
		left_over.tv_usec -= 1000000;
	}
	if (left_over.tv_usec >= 500000)
		left_over.tv_sec++;
	return(left_over.tv_sec);
}

static
sleepx()
{
}

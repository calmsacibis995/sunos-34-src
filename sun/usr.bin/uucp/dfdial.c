/*	dfdial.c	1.1	86/09/25	*/

#include "uucp.h"
#include "dial.h"
#include <signal.h>
#include <sgtty.h>
#include <setjmp.h>

jmp_buf Sjbuf;

/***
 *	dfdial(d, ph)	use DF02/3 to dial remote machine
 *	struct Devices *d;
 *	char *ph;
 *
 *	return codes:
 *		file descriptor  -  succeeded
 *		FAIL  -  failed
 */

dfdial(d, ph)
struct Devices *d;
char *ph;
{
	char phone[MAXPH+2], c = 0;
	int dcf;
	unsigned timelim;
	FILE *dfp;
	int df03 = prefix("ACUDF03", d->D_type);


#ifdef TIOCMSET
	if (df03) {
		int st = TIOCM_ST;

		fixline(Dnf, 1200);
		if (d->D_speed != 1200)
			ioctl(Dnf, TIOCMBIC, &st);
		else
			ioctl(Dnf, TIOCMBIS, &st);
	} else
#endif
	fixline(Dnf, d->D_speed);
	sprintf(phone, "\002%s", ph);
	if (setjmp(Sjbuf)) {
		DEBUG(1, "DF write %s\n", "timeout");
		logent("DIALUP DF write", "TIMEOUT");
		delock(d->D_line);
		if (Dnf)
			close(Dnf);
		return(FAIL);
	}
	signal(SIGALRM, alarmtr);
	timelim = 5 * strlen(phone);
	alarm(timelim < 30 ? 30 : timelim);
	ioctl(Dnf, TIOCFLUSH, STBNULL);
	write(Dnf, "\001", 1);
	sleep(1);
	write(Dnf, phone, strlen(phone));
	DEBUG(4, "ACU write ok%s\n", "");
	if (read(Dnf, &c, 1) != 1 || c != 'A') {
		DEBUG(4, "c is %o\n", c);
		DEBUG(1, "Line open %s\n", "failed");
		logent("DIALUP LINE open", "FAILED");
		alarm(0);
		close(Dnf);
		delock(d->D_line);
		return(FAIL);
	}
	alarm(0);
	dcf = Dnf;
	Dnf = 0;
	ioctl(dcf, TIOCHPCL, STBNULL);
	fflush(stdout);
	fixline(dcf, d->D_speed);
	return(dcf);
}

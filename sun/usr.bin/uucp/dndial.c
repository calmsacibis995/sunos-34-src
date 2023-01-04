/*	dndial.c	1.1	86/09/25	*/

#include "uucp.h"
#include "dial.h"
#include <signal.h>
#include <sgtty.h>
#include <setjmp.h>

jmp_buf Sjbuf;

/***
 *	dndial(d, ph)	use DN11 to dial remote machine
 *	struct Devices *d;
 *	char *ph;
 *
 *	return codes:
 *		file descriptor  -  succeeded
 *		FAIL  -  failed
 */

dndial(d, ph)
struct Devices *d;
char *ph;
{
	char dcname[20], dnname[20], phone[MAXPH+2];
	int nw, lt, pid, dcf;
	unsigned timelim;
	FILE *dfp;

	sprintf(dcname, "/dev/%s", d->D_line);
	sprintf(phone, "%s%s", ph, ACULAST);
	DEBUG(4, "dc - %s, ", dcname);
	DEBUG(4, "acu - %s\n", dnname);
	if (setjmp(Sjbuf)) {
		DEBUG(1, "DN write %s\n", "timeout");
		logent("DIALUP DN write", "TIMEOUT");
		kill(pid, 9);
		delock(d->D_line);
		if (Dnf)
			close(Dnf);
		return(FAIL);
	}
	signal(SIGALRM, alarmtr);
	timelim = 5 * strlen(phone);
	alarm(timelim < 30 ? 30 : timelim);
	if ((pid = fork()) == 0) {
		sleep(2);
		fclose(stdin);
		fclose(stdout);
		nw = write(Dnf, phone, lt = strlen(phone));
		if (nw != lt) {
			DEBUG(1, "ACU write %s\n", "error");
			logent("DIALUP ACU write", "FAILED");
			exit(1);
		}
		DEBUG(4, "ACU write ok%s\n", "");
		exit(0);
	}
	/*  open line - will return on carrier */
	/* RT needs a sleep here because it returns immediately from open */

#if RT
	sleep(15);
#endif

	dcf = open(dcname, 2);
	DEBUG(4, "dcf is %d\n", dcf);
	if (dcf < 0) {
		DEBUG(1, "Line open %s\n", "failed");
		logent("DIALUP LINE open", "FAILED");
		alarm(0);
		kill(pid, 9);
		close(Dnf);
		delock(d->D_line);
		return(FAIL);
	}
	ioctl(dcf, TIOCHPCL, STBNULL);
	while ((nw = wait(&lt)) != pid && nw != -1)
		;
	alarm(0);
	fflush(stdout);
	fixline(dcf, d->D_speed);
	DEBUG(4, "Fork Stat %o\n", lt);
	if (lt != 0) {
		close(dcf);
		if (Dnf)
			close(Dnf);
		delock(d->D_line);
		return(FAIL);
	}
	return(dcf);
}

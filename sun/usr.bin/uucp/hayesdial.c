/*	hayesdial.c	1.1	86/09/25	*/

#include "uucp.h"
#include "dial.h"
#include <signal.h>
#include <sgtty.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/timeb.h>

jmp_buf Sjbuf;

char *hayes_err[] = {
	"OK",
	"CONNECT",
	"RING",
	"NO CARRIER",
	"ERROR",
	"CONNECT 1200",
	"NO DIALTONE",
	"BUSY",
	"NO ANSWER",
	"CONNECT 0600",
	"CONNECT 1200",
};

/***
 *	hayesdial(d, ph)	use Hayes SmartModem 1200 or 2400 to dial
 *	struct Devices *d;
 *	char *ph;
 *
 *	return codes:
 *		file descriptor  -  succeeded
 *		FAIL  -  failed
 */

hayesdial(d, ph)
struct Devices *d;
char *ph;
{
	register char *p;
	int dcf;
	int zero = 0;	/* for TIOCFLUSH */
	char *err = "DIALUP LINE open";
	extern char *expectline();

	fixline(Dnf, d->D_speed);
	if (setjmp(Sjbuf)) {
		DEBUG(1, "Hayes write %s\n", "timeout");
		logent("DIALUP Hayes write", "TIMEOUT");
		delock(d->D_line);
		if (Dnf)
			close(Dnf);
		return(FAIL);
	}
	ioctl(Dnf, TIOCCDTR, 0);
	sleep(1);
	ioctl(Dnf, TIOCSDTR, 0);
	DEBUG(4, "dialing Hayes\n", "");
	write(Dnf, "ATV0Q0E0S0=1S2=255S12=255\r", 26);
	sleep(1);
	/* flush any echoes or return codes */
	ioctl(Dnf, TIOCFLUSH, &zero);
	/* now see if the modem is talking to us properly */
	write(Dnf, "AT\r", 3);
	if (expect("0\r", Dnf))
		goto out;
	ioctl(Dnf, TIOCFLUSH, &zero);
	if (*ph == 'S')
		write(Dnf, "AT", 2);
	else
		write(Dnf, "ATDT", 4);
	write(Dnf, ph, strlen(ph));
	write(Dnf, "\r", 1);
	DEBUG(4, "ACU write ok%s\n", "");
	p = expectline(Dnf);
	if (p == NULL) {
		err = "DIALER FAILED";
		goto out;
	}
	if (strcmp(p, "1") == 0 || strcmp(p, "5") == 0) {
		/* connected at 1200 baud, were we trying for 2400? */
		if (d->D_speed == 2400) {
			fixline(Dnf, 1200);
			DEBUG(1, "Fallback from 2400 baud to 1200 baud\n", "");
			logent("from 2400 to 1200", "FALLBACK");
		}
	} else if (strcmp(p, "10") == 0) {
		/* connected at 2400 baud, all ok */
		;
	} else if (*p >= '0' && *p <= '9' && p[1] == '\0') {
		err = hayes_err[*p - '0'];
		goto out;
	} else {
		err = "UNKNOWN DIALER ERROR";
out:
		DEBUG(1, "Line open failed, %s\n", err);
		logent(err, "FAILED");
		close(Dnf);
		delock(d->D_line);
		return(FAIL);
	}
	dcf = Dnf;
	Dnf = 0;
	ioctl(dcf, TIOCHPCL, STBNULL);
	fflush(stdout);
	return(dcf);
}

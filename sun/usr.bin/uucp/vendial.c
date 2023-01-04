/*	vendial.c	1.1	86/09/25	*/

#include "uucp.h"
#include "dial.h"
#include <signal.h>
#include <sgtty.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/timeb.h>

jmp_buf Sjbuf;

/***
 *	vendial(d, ph)	use Ventel MD212+ to dial remote machine
 *	struct Devices *d;
 *	char *ph;
 *
 *	return codes:
 *		file descriptor  -  succeeded
 *		FAIL  -  failed
 */

vendial(d, ph)
struct Devices *d;
char *ph;
{
	register char *p;
	register int i;
	int dcf;
	FILE *dfp;
	struct timeb loctime;
	extern char *expectline();

	fixline(Dnf, d->D_speed);
	if (setjmp(Sjbuf)) {
		DEBUG(1, "Ventel write %s\n", "timeout");
		logent("DIALUP Ventel write", "TIMEOUT");
		delock(d->D_line);
		if (Dnf)
			close(Dnf);
		return(FAIL);
	}
	DEBUG(4, "dialing Ventel\n", "");
	ftime(&loctime);
	write(Dnf, "\r", 1);
	i = loctime.millitm;
	while (abs(loctime.millitm - i) < 250)
		ftime(&loctime);
	write(Dnf, "\r", 1);
	if (expect("\r\n$", Dnf))
		goto out;
	write(Dnf, "K", 1);
	if (expect("DIAL:", Dnf))
		goto out;
	ioctl(Dnf, TIOCFLUSH, STBNULL);
	write(Dnf, "<\0\0", 3);
	for (p = ph; *p; p++) {
		write(Dnf, p, 1);
		write(Dnf, "\0\0\0", 3);
	}
	write(Dnf, "%\0\0>\0\0\r", 7);
	DEBUG(4, "ACU write ok%s\n", "");
	if (expect("DIALING", Dnf)) {
out:
		DEBUG(1, "Line open %s\n", "failed");
		logent("DIALUP LINE open", "FAILED");
		close(Dnf);
		delock(d->D_line);
		return(FAIL);
	}
	if ((p = expectline(Dnf)) == NULL) {
		goto out;
	}
	i = strlen(p);
	if (i < 7 || strcmp(&p[i - 7], "ONLINE!") != 0) {
		/* extract the error message */
		register char *s;

		for (s = p + i - 1; s > p; s--)
			if (*s == ' ' && s[1] == ' ') {
				s += 2;
				break;
			}
		DEBUG(1, "Line open failed %s\n", s);
		logent(s, "FAILED");
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

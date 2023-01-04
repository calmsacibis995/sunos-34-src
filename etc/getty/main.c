#ifndef lint
static	char sccsid[] = "@(#)main.c 1.4 86/12/15 SMI"; /* from UCB 4.5 83/08/01 */
#endif

/*
 * getty -- adapt to terminal speed on dialup, and call login
 *
 * Melbourne getty, June 83, kre.
 */

#include <sgtty.h>
#include <signal.h>
#include <ctype.h>
#include <setjmp.h>
#include "gettytab.h"

struct	sgttyb tmode = {
	0, 0, CERASE, CKILL, 0
};
struct	tchars tc = {
	CINTR, CQUIT, CSTART,
	CSTOP, CEOF, CBRK,
};
struct	ltchars ltc = {
	CSUSP, CDSUSP, CRPRNT,
	CFLUSH, CWERASE, CLNEXT
};

int	crmod;
int	upper;
int	lower;
int	digit;

char	hostname[32];
char	name[16];
char	*portselector();

#define	OBUFSIZ		128
#define	TABBUFSIZ	512

char	defent[TABBUFSIZ];
char	defstrs[TABBUFSIZ];
char	tabent[TABBUFSIZ];
char	tabstrs[TABBUFSIZ];

char	*env[128];

char partab[] = {
	0001,0201,0201,0001,0201,0001,0001,0201,
	0202,0004,0003,0205,0005,0206,0201,0001,
	0201,0001,0001,0201,0001,0201,0201,0001,
	0001,0201,0201,0001,0201,0001,0001,0201,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0201
};

#define	ERASE	tmode.sg_erase
#define	KILL	tmode.sg_kill
#define	EOT	tc.t_eofc

jmp_buf timeout;

dingdong()
{

	alarm(0);
	signal(SIGALRM, SIG_DFL);
	longjmp(timeout, 1);
}

jmp_buf	intrupt;

interrupt()
{

	signal(SIGINT, interrupt);
	longjmp(intrupt, 1);
}

main(argc, argv)
	char *argv[];
{
	char *tname;
	long allflags;

	signal(SIGINT, SIG_IGN);
/*
	signal(SIGQUIT, SIG_DFL);
*/
	gethostname(hostname, sizeof(hostname));
	if (hostname[0] == '\0')
		strcpy(hostname, "Amnesiac");
	gettable("default", defent, defstrs);
	gendefaults();
	tname = "default";
	if (argc > 1)
		tname = argv[1];
	for (;;) {
		int ldisp = OTTYDISC;
		int off = 0;

		gettable(tname, tabent, tabstrs);
		if (OPset || EPset || APset)
			APset++, OPset++, EPset++;
		setdefaults();
		ioctl(0, TIOCFLUSH, 0);		/* clear out the crap */
		ioctl(0, FIONBIO, &off);
		if (IS)
			tmode.sg_ispeed = speed(IS);
		else if (SP)
			tmode.sg_ispeed = speed(SP);
		if (OS)
			tmode.sg_ospeed = speed(OS);
		else if (SP)
			tmode.sg_ospeed = speed(SP);
		tmode.sg_flags = setflags(0);
		ioctl(0, TIOCSETP, &tmode);
		setchars();
		ioctl(0, TIOCSETC, &tc);
		ioctl(0, TIOCSETD, &ldisp);
		if (HC)
			ioctl(0, TIOCHPCL, 0);
		if (PS) {
			tname = portselector();
			continue;
		}
		if (CL && *CL)
			putpad(CL);
		edithost(HE);
		if (IM && *IM)
			putf(IM);
		if (setjmp(timeout)) {
			tmode.sg_ispeed = tmode.sg_ospeed = 0;
			ioctl(0, TIOCSETP, &tmode);
			exit(1);
		}
		if (TO) {
			signal(SIGALRM, dingdong);
			alarm(TO);
		}
		if (getname()) {
			alarm(0);
			signal(SIGALRM, SIG_DFL);
			if (!(upper || lower || digit))
				continue;
			allflags = setflags(2);
			tmode.sg_flags = allflags & 0xffff;
			allflags >>= 16;
			if (crmod || NL)
				tmode.sg_flags |= CRMOD;
			if (upper || UC)
				tmode.sg_flags |= LCASE;
			if (lower || LC)
				tmode.sg_flags &= ~LCASE;
			ioctl(0, TIOCSETP, &tmode);
			ioctl(0, TIOCSLTC, &ltc);
			ioctl(0, TIOCLSET, &allflags);
			putchr('\n');
			oflush();
			makeenv(env);
			signal(SIGINT, SIG_DFL);
			execle(LO, "login", name, (char *)0, env);
			exit(1);
		}
		alarm(0);
		signal(SIGALRM, SIG_DFL);
		signal(SIGINT, SIG_IGN);
		if (NX && *NX)
			tname = NX;
	}
}

getname()
{
	register char *np;
	register c;
	char cs;

	/*
	 * Interrupt may happen if we use CBREAK mode
	 */
	if (setjmp(intrupt)) {
		signal(SIGINT, SIG_IGN);
		return (0);
	}
	signal(SIGINT, interrupt);
	tmode.sg_flags = setflags(0);
	ioctl(0, TIOCSETP, &tmode);
	tmode.sg_flags = setflags(1);
	prompt();
	if (PF > 0) {
		oflush();
		sleep(PF);
		PF = 0;
	}
	ioctl(0, TIOCSETP, &tmode);
	crmod = 0;
	upper = 0;
	lower = 0;
	digit = 0;
	np = name;
	for (;;) {
		oflush();
		if (read(0, &cs, 1) <= 0)
			exit(0);
		if ((c = cs&0177) == 0)
			return (0);
		if (c == EOT)
			exit(1);
		if (c == '\r' || c == '\n' || np >= &name[16])
			break;

		if (c >= 'a' && c <= 'z')
			lower++;
		else if (c >= 'A' && c <= 'Z') {
			upper++;
		} else if (c == ERASE || c == '#' || c == '\b') {
			if (np > name) {
				np--;
				if (tmode.sg_ospeed >= B1200)
					puts("\b \b");
				else
					putchr(cs);
			}
			continue;
		} else if (c == KILL || c == '@') {
			putchr(cs);
			putchr('\r');
			if (tmode.sg_ospeed < B1200)
				putchr('\n');
			/* this is the way they do it down under ... */
			else if (np > name)
				puts("                                     \r");
			prompt();
			np = name;
			continue;
		} else if (c == ' ')
			c = '_';
		else if (c >= '0' && c <= '9')
			digit++;
		if (IG && (c < ' ' || c > 0176))
			continue;
		*np++ = c;
		putchr(cs);
	}
	signal(SIGINT, SIG_IGN);
	*np = 0;
	if (c == '\r')
		crmod++;
	if (upper && !lower && !LC || UC)
		for (np = name; *np; np++)
			if (isupper(*np))
				*np = tolower(*np);
	return (1);
}

static
short	tmspc10[] = {
	0, 2000, 1333, 909, 743, 666, 500, 333, 166, 83, 55, 41, 20, 10, 5, 15
};

putpad(s)
	register char *s;
{
	register pad = 0;
	register mspc10;

	if (isdigit(*s)) {
		while (isdigit(*s)) {
			pad *= 10;
			pad += *s++ - '0';
		}
		pad *= 10;
		if (*s == '.' && isdigit(s[1])) {
			pad += s[1] - '0';
			s += 2;
		}
	}

	puts(s);
	/*
	 * If no delay needed, or output speed is
	 * not comprehensible, then don't try to delay.
	 */
	if (pad == 0)
		return;
	if (tmode.sg_ospeed <= 0 ||
	    tmode.sg_ospeed >= (sizeof tmspc10 / sizeof tmspc10[0]))
		return;

	/*
	 * Round up by a half a character frame,
	 * and then do the delay.
	 * Too bad there are no user program accessible programmed delays.
	 * Transmitting pad characters slows many
	 * terminals down and also loads the system.
	 */
	mspc10 = tmspc10[tmode.sg_ospeed];
	pad += mspc10 / 2;
	for (pad /= mspc10; pad > 0; pad--)
		putchr(*PC);
}

puts(s)
	register char *s;
{

	while (*s)
		putchr(*s++);
}

char	outbuf[OBUFSIZ];
int	obufcnt = 0;

putchr(cc)
{
	char c;

	c = cc;
	c |= partab[c&0177] & 0200;
	if (OP)
		c ^= 0200;
	if (!UB) {
		outbuf[obufcnt++] = c;
		if (obufcnt >= OBUFSIZ)
			oflush();
	} else
		write(1, &c, 1);
}

oflush()
{
	if (obufcnt)
		write(1, outbuf, obufcnt);
	obufcnt = 0;
}

prompt()
{

	putf(LM);
	if (CO)
		putchr('\n');
}

putf(cp)
	register char *cp;
{
	extern char editedhost[];

	while (*cp) {
		if (*cp != '%') {
			putchr(*cp++);
			continue;
		}
		switch (*++cp) {

		case 'h':
			puts(editedhost);
			break;

		case '%':
			putchr('%');
			break;
		}
		cp++;
	}
}

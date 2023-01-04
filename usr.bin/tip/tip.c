#ifndef lint
static	char sccsid[] = "@(#)tip.c 1.1 86/09/25 SMI"; /* from UCB 4.16 6/28/83 */
#endif

/*
 * tip - UNIX link to other systems
 *  tip [-v] [-speed] system-name
 * or
 *  cu phone-number [-s speed] [-l line] [-a acu]
 */
#include "tip.h"

/*
 * Baud rate mapping table
 */
int bauds[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600,
	1200, 1800, 2400, 4800, 9600, 19200, -1
};

int	disc = OTTYDISC;		/* tip normally runs this way */
int	intprompt();
int	timeout();
int	cleanup();
char	*sname();

main(argc, argv)
	char *argv[];
{
	char *system = NOSTR;
	register int i;
	register char *p;
	char sbuf[12];

	if (equal(sname(argv[0]), "cu")) {
		cumain(argc, argv);
		cumode = 1;
		goto cucommon;
	}

	if (argc > 4) {
		fprintf(stderr, "usage: tip [-v] [-speed] [system-name]\n");
		exit(1);
	}
	if (!isatty(0)) {
		fprintf(stderr, "tip: must be interactive\n");
		exit(1);
	}

	for (; argc > 1; argv++, argc--) {
		if (argv[1][0] != '-')
			system = argv[1];
		else switch (argv[1][1]) {

		case 'v':
			vflag++;
			break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			BR = atoi(&argv[1][1]);
			break;

		default:
			fprintf(stderr, "tip: %s, unknown option\n", argv[1]);
			break;
		}
	}

	if (system == NOSTR)
		goto notnumber;
	for (p = system; *p; p++)
		if (isalpha(*p))
			goto notnumber;
	PN = system;		/* system name is really a phone number */
	sprintf(sbuf, "tip%d", BR);
	system = sbuf;

notnumber:
	signal(SIGINT, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGHUP, cleanup);
	signal(SIGTERM, cleanup);

	if ((i = hunt(system)) == 0) {
		printf("all ports busy\n");
		exit(3);
	}
	if (i == -1) {
		printf("link down\n");
		delock(uucplock);
		exit(3);
	}
	setbuf(stdout, NULL);
	loginit();
	/*
	 * Kludge, their's no easy way to get the initialization
	 *   in the right order, so force it here.
	 * Do the open here, before we change back to real uid.
	 * We will check whether the open succeeded later, when
	 * (and if) we actually go to use the file.
	 */
	if ((PH = getenv("PHONES")) == NOSTR)
		PH = "/etc/phones";
	phfd = fopen(PH, "r");
	/*
	 * Now that we have the logfile and the ACU open
	 *  return to the real uid and gid.  These things will
	 *  be closed on exit.  Note that we can't run as root,
	 *  because locking mechanism on the tty and the accounting
	 *  will be bypassed.
	 */
	setgid(getgid());
	setuid(getuid());

	vinit();				/* init variables */
	setparity("none");			/* set the parity table */
	if ((i = speed(number(value(BAUDRATE)))) == NULL) {
		printf("tip: bad baud rate %d\n", number(value(BAUDRATE)));
		delock(uucplock);
		exit(3);
	}

	/*
	 * Hardwired connections require the
	 *  line speed set before they make any transmissions
	 *  (this is particularly true of things like a DF03-AC)
	 */
	if (HW)
		ttysetup(i);
	if (p = connect()) {
		printf("\07%s\n[EOT]\n", p);
		delock(uucplock);
		exit(1);
	}
	if (!HW)
		ttysetup(i);
cucommon:
	/*
	 * From here down the code is shared with
	 * the "cu" version of tip.
	 */
	ioctl(0, TIOCGETP, (char *)&defarg);
	ioctl(0, TIOCGETC, (char *)&defchars);
	ioctl(0, TIOCGLTC, (char *)&deflchars);
	ioctl(0, TIOCGETD, (char *)&odisc);
	arg = defarg;
	arg.sg_flags = ANYP | CBREAK;
	tchars = defchars;
	tchars.t_intrc = tchars.t_quitc = -1;
	/* handle tandem mode in case was set in remote file */
	if (boolean(value(TAND)))
		tandem("on");
	else
		tandem("off");
	ltchars = deflchars;
	ltchars.t_suspc = ltchars.t_dsuspc = ltchars.t_flushc
		= ltchars.t_lnextc = -1;
	raw();

	pipe(fildes); pipe(repdes);
	signal(SIGALRM, timeout);

	/*
	 * Everything's set up now:
	 *	connection established (hardwired or diaulup)
	 *	line conditioned (baud rate, mode, etc.)
	 *	internal data structures (variables)
	 * so, fork one process for local side and one for remote.
	 */
	if (CM != NOSTR) {
		sleep(2);	/* let line settle */
		pwrite(FD, CM, strlen(CM));
	}
	printf(cumode ? "Connected\r\n" : "\07connected\r\n");
	if (pid = fork())
		tipin();
	else
		tipout();
	/*NOTREACHED*/
}

cleanup()
{

	delock(uucplock);
	if (odisc)
		ioctl(0, TIOCSETD, (char *)&odisc);
	exit(0);
}

/*
 * put the controlling keyboard into raw mode
 */
raw()
{

	ioctl(0, TIOCSETP, &arg);
	ioctl(0, TIOCSETC, &tchars);
	ioctl(0, TIOCSLTC, &ltchars);
	ioctl(0, TIOCSETD, (char *)&disc);
}


/*
 * return keyboard to normal mode
 */
unraw()
{

	ioctl(0, TIOCSETD, (char *)&odisc);
	ioctl(0, TIOCSETP, (char *)&defarg);
	ioctl(0, TIOCSETC, (char *)&defchars);
	ioctl(0, TIOCSLTC, (char *)&deflchars);
}

static	jmp_buf promptbuf;

/*
 * Print string ``s'', then read a string
 *  in from the terminal.  Handles signals & allows use of
 *  normal erase and kill characters.
 */
prompt(s, p)
	char *s;
	register char *p;
{
	register char *b = p;
	int (*ointr)(), (*oquit)();

	stoprompt = 0;
	ointr = signal(SIGINT, intprompt);
	oquit = signal(SIGQUIT, SIG_IGN);
	unraw();
	printf("%s", s);
	if (setjmp(promptbuf) == 0)
		while ((*p = getchar()) != EOF && *p != '\n')
			p++;
	*p = '\0';

	raw();
	signal(SIGINT, ointr);
	signal(SIGQUIT, oquit);
	return (stoprompt || p == b);
}

/*
 * Interrupt service routine during prompting
 */
intprompt()
{

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	stoprompt = 1;
	printf("\r\n");
	longjmp(promptbuf, 1);
}

/*
 * ****TIPIN   TIPIN****
 */
tipin()
{
	char gch, bol = 1;

	/*
	 * Kinda klugey here...
	 *   check for scripting being turned on from the .tiprc file,
	 *   but be careful about just using setscript(), as we may
	 *   send a SIGEMT before tipout has a chance to set up catching
	 *   it; so wait a second, then setscript()
	 */
	if (boolean(value(SCRIPT))) {
		sleep(1);
		setscript();
	}

	while (1) {
		gch = getchar()&0177;
		if ((gch == character(value(ESCAPE))) && bol) {
			if (!(gch = escape()))
				continue;
		} else if (!cumode && gch == character(value(RAISECHAR))) {
			boolean(value(RAISE)) = !boolean(value(RAISE));
			continue;
		} else if (gch == '\r') {
			bol = 1;
			pwrite(FD, &gch, 1);
			if (boolean(value(HALFDUPLEX)))
				printf("\r\n");
			continue;
		} else if (!cumode && gch == character(value(FORCE)))
			gch = getchar()&0177;
		bol = any(gch, value(EOL));
		if (boolean(value(RAISE)) && islower(gch))
			gch = toupper(gch);
		pwrite(FD, &gch, 1);
		if (boolean(value(HALFDUPLEX)))
			printf("%c", gch);
	}
}

/*
 * Escape handler --
 *  called on recognition of ``escapec'' at the beginning of a line
 */
escape()
{
	register char gch;
	register esctable_t *p;
	char c = character(value(ESCAPE));
	extern esctable_t etable[];

	gch = (getchar()&0177);
	for (p = etable; p->e_char; p++)
		if (p->e_char == gch) {
			if ((p->e_flags&PRIV) && getuid())
				continue;
			printf("%s", ctrl(c));
			(*p->e_func)(gch);
			return (0);
		}
	/* ESCAPE ESCAPE forces ESCAPE */
	if (c != gch)
		pwrite(FD, &c, 1);
	return (gch);
}

speed(n)
	int n;
{
	register int *p;

	for (p = bauds; *p != -1;  p++)
		if (*p == n)
			return (p - bauds);
	return (NULL);
}

any(c, p)
	register char c, *p;
{
	while (p && *p)
		if (*p++ == c)
			return (1);
	return (0);
}

char *
interp(s)
	register char *s;
{
	static char buf[256];
	register char *p = buf, c, *q;

	while (c = *s++) {
		for (q = "\nn\rr\tt\ff\033E\bb"; *q; q++)
			if (*q++ == c) {
				*p++ = '\\'; *p++ = *q;
				goto next;
			}
		if (c < 040) {
			*p++ = '^'; *p++ = c + 'A'-1;
		} else if (c == 0177) {
			*p++ = '^'; *p++ = '?';
		} else
			*p++ = c;
	next:
		;
	}
	*p = '\0';
	return (buf);
}

char *
ctrl(c)
	char c;
{
	static char s[3];

	if (c < 040 || c == 0177) {
		s[0] = '^';
		s[1] = c == 0177 ? '?' : c+'A'-1;
		s[2] = '\0';
	} else {
		s[0] = c;
		s[1] = '\0';
	}
	return (s);
}

/*
 * Help command
 */
help(c)
	char c;
{
	register esctable_t *p;
	extern esctable_t etable[];

	printf("%c\r\n", c);
	for (p = etable; p->e_char; p++) {
		if ((p->e_flags&PRIV) && getuid())
			continue;
		printf("%2s", ctrl(character(value(ESCAPE))));
		printf("%-2s %c   %s\r\n", ctrl(p->e_char),
			p->e_flags&EXP ? '*': ' ', p->e_help);
	}
}

/*
 * Set up the "remote" tty's state
 */
ttysetup(speed)
	int speed;
{
	unsigned bits = LDECCTQ;
	struct sgttyb buf;

	ioctl(FD, TIOCGETP, (char *)&buf);
	buf.sg_ispeed = buf.sg_ospeed = speed;
	buf.sg_flags = CBREAK | (buf.sg_flags&ANYP);
	if (boolean(value(TAND)))
		buf.sg_flags |= TANDEM;
	ioctl(FD, TIOCSETP, (char *)&buf);
	ioctl(FD, TIOCLBIS, (char *)&bits);
}

/*
 * Return "simple" name from a file name,
 * strip leading directories.
 */
char *
sname(s)
	register char *s;
{
	register char *p = s;

	while (*s)
		if (*s++ == '/')
			p = s;
	return (p);
}

static char partab[0200];

/*
 * Do a write to the remote machine with the correct parity.
 * We are doing 8 bit wide output, so we just generate a character
 * with the right parity and output it.
 */
pwrite(fd, buf, n)
	int fd;
	char *buf;
	register int n;
{
	register int i;
	register char *bp;

	bp = buf;
	for (i = 0; i < n; i++) {
		*bp = partab[(*bp) & 0177];
		bp++;
	}
	write(fd, buf, n);
}

/*
 * Build a parity table with appropriate high-order bit.
 */
setparity(defparity)
	char *defparity;
{
	register int i;
	char *parity;
	struct sgttyb buf;
	extern char evenpartab[];

	if (value(PARITY) == NOSTR)
		value(PARITY) = defparity;
	parity = value(PARITY);
	ioctl(FD, TIOCGETP, (char *)&buf);
	buf.sg_flags &= ~ANYP;			/* turn off all parity */
	for (i = 0; i < 0200; i++)
		partab[i] = evenpartab[i];
	if (equal(parity, "even"))
		buf.sg_flags |= EVENP;
	else if (equal(parity, "odd")) {
		for (i = 0; i < 0200; i++)
			partab[i] ^= 0200;	/* reverse bit 7 */
		buf.sg_flags |= ODDP;
	} else if (equal(parity, "none") || equal(parity, "zero")) {
		for (i = 0; i < 0200; i++)
			partab[i] &= ~0200;	/* turn off bit 7 */
	} else if (equal(parity, "one")) {
		for (i = 0; i < 0200; i++)
			partab[i] |= 0200;	/* turn on bit 7 */
		return;
	} else {
		fprintf(stderr, "%s: unknown parity value\n", PA);
		fflush(stderr);
		return;
	}
	ioctl(FD, TIOCSETP, (char *)&buf);
}

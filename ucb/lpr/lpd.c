#ifndef lint
static	char sccsid[] = "@(#)lpd.c 1.1 86/09/25 SMI"; /* from UCB 4.10 08/24/83 */
#endif

/*
 * lpd -- line printer daemon.
 *
 * Listen for a connection and perform the requested operation.
 * Operations are:
 *	\1printer\n
 *		check the queue for jobs and print any found.
 *	\2printer\n
 *		receive a job from another machine and queue it.
 *	\3printer [users ...] [jobs ...]\n
 *		return the current state of the queue (short form).
 *	\4printer [users ...] [jobs ...]\n
 *		return the current state of the queue (long form).
 *	\5printer person [users ...] [jobs ...]\n
 *		remove jobs from the queue.
 *	\6printer\n
 *		enable queuing on the specified printer queue.
 *	\7printer\n
 *		disable queuing on the specified printer queue.
 *	\8printer\n
 *		return the queue status (queuing enabled or disabled).
 *
 * Strategy to maintain protected spooling area:
 *	1. Spooling area is writable only by daemon and spooling group
 *	2. lpr runs setuid root and setgrp spooling group; it uses
 *	   root to access any file it wants (verifying things before
 *	   with an access call) and group id to know how it should
 *	   set up ownership of files in the spooling area.
 *	3. Files in spooling area are owned by root, group spooling
 *	   group, with mode 660.
 *	4. lpd, lpq and lprm run setuid daemon and setgrp spooling group to
 *	   access files and printer.  Users can't get to anything
 *	   w/o help of lpq and lprm programs.
 */

#include "lp.h"

static int	lflag;				/* log requests flag */
static char	*logfile = DEFLOGF;

int	reapchild();
int	cleanup();

main(argc, argv)
	int argc;
	char **argv;
{
	int f, funix, finet, options, defreadfds, fromlen;
	struct sockaddr_un s_un, fromunix;
	struct sockaddr_in sin, frominet;
	int omask, lfd;

	gethostname(host, sizeof(host));
	name = argv[0];

	while (--argc > 0) {
		argv++;
		if (argv[0][0] == '-')
			switch (argv[0][1]) {
			case 'd':
				options |= SO_DEBUG;
				break;
			case 'l':
				lflag++;
				break;
			case 'L':
				argc--;
				logfile = *++argv;
				break;
			}
	}

#ifndef DEBUG
	/*
	 * Set up standard environment by detaching from the parent.
	 */
	if (fork())
		exit(0);
	for (f = 0; f < 3; f++)
		(void) close(f);
	(void) open("/dev/null", O_RDONLY);
	(void) open("/dev/null", O_WRONLY);
	(void) open(logfile, O_WRONLY|O_APPEND);
	f = open("/dev/tty", O_RDWR);
	if (f > 0) {
		ioctl(f, TIOCNOTTY, 0);
		(void) close(f);
	}
#endif

	(void) umask(0);
	lfd = open(MASTERLOCK, O_WRONLY|O_CREAT, 0644);
	if (lfd < 0) {
		log("cannot create %s", MASTERLOCK);
		exit(1);
	}
	if (flock(lfd, LOCK_EX|LOCK_NB) < 0) {
		if (errno == EWOULDBLOCK)	/* active deamon present */
			exit(0);
		log("cannot lock %s", MASTERLOCK);
		exit(1);
	}
	ftruncate(lfd, 0);
	/*
	 * write process id for others to know
	 */
	sprintf(line, "%u\n", getpid());
	f = strlen(line);
	if (write(lfd, line, f) != f) {
		log("cannot write daemon pid");
		exit(1);
	}
	signal(SIGCHLD, reapchild);
	/*
	 * Restart all the printers.
	 */
	startup();
	(void) unlink(SOCKETNAME);
	funix = socket(AF_UNIX, SOCK_STREAM, 0);
	if (funix < 0) {
		logerr("socket");
		exit(1);
	}
#define	mask(s)	(1 << ((s) - 1))
	omask = sigblock(mask(SIGHUP)|mask(SIGINT)|mask(SIGQUIT)|mask(SIGTERM));
	signal(SIGHUP, cleanup);
	signal(SIGINT, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGTERM, cleanup);
	s_un.sun_family = AF_UNIX;
	strcpy(s_un.sun_path, SOCKETNAME);
	if (bind(funix, &s_un, strlen(s_un.sun_path) + 2) < 0) {
		logerr("unix domain bind");
		exit(1);
	}
	sigsetmask(omask);
	defreadfds = 1 << funix;
	listen(funix, 5);
	finet = socket(AF_INET, SOCK_STREAM, 0);
	if (finet >= 0) {
		struct servent *sp;

		if (options & SO_DEBUG)
			if (setsockopt(finet, SOL_SOCKET, SO_DEBUG, 0, 0) < 0) {
				logerr("setsockopt (SO_DEBUG)");
				cleanup();
			}
		sp = getservbyname("printer", "tcp");
		if (sp == NULL) {
			log("printer/tcp: unknown service");
			cleanup();
		}
		sin.sin_family = AF_INET;
		sin.sin_port = sp->s_port;
		if (bind(finet, &sin, sizeof(sin), 0) < 0) {
			logerr("internet domain bind");
			cleanup();
		}
		defreadfds |= 1 << finet;
		listen(finet, 5);
	}
	/*
	 * Main loop: accept, do a request, continue.
	 */
	for (;;) {
		int domain, nfds, s, readfds = defreadfds;

		nfds = select(20, &readfds, 0, 0, 0);
		if (nfds <= 0) {
			if (nfds < 0 && errno != EINTR) {
				logerr("select");
				cleanup();
				/*NOTREACHED*/
			}
			continue;
		}
		if (readfds & (1 << funix)) {
			domain = AF_UNIX, fromlen = sizeof(fromunix);
			s = accept(funix, &fromunix, &fromlen);
		} else if (readfds & (1 << finet)) {
			domain = AF_INET, fromlen = sizeof(frominet);
			s = accept(finet, &frominet, &fromlen);
		}
		if (s < 0) {
			if (errno == EINTR)
				continue;
			logerr("accept");
			cleanup();
		}
		if (fork() == 0) {
			signal(SIGCHLD, SIG_IGN);
			signal(SIGHUP, SIG_IGN);
			signal(SIGINT, SIG_IGN);
			signal(SIGQUIT, SIG_IGN);
			signal(SIGTERM, SIG_IGN);
			(void) close(funix);
			(void) close(finet);
			dup2(s, 1);
			(void) close(s);
			if (domain == AF_INET)
				chkhost(&frominet);
			doit();
			exit(0);
		}
		(void) close(s);
	}
}

static
reapchild()
{
	union wait status;

	while (wait3(&status, WNOHANG, 0) > 0)
		;
}

static
cleanup()
{
	if (lflag)
		log("cleanup()");
	unlink(SOCKETNAME);
	exit(0);
}

/*
 * Stuff for handling job specifications
 */
char	*user[MAXUSERS];	/* users to process */
int	users;			/* # of users in user array */
int	requ[MAXREQUESTS];	/* job number of spool entries */
int	requests;		/* # of spool requests */
char	*person;		/* name of person doing lprm */

static char	fromb[32];	/* buffer for client's machine name */
static char	cbuf[BUFSIZ];	/* command line buffer */
static char	*cmdnames[] = {
	"null",
	"printjob",
	"recvjob",
	"displayq short",
	"displayq long",
	"rmjob"
};

static
doit()
{
	register char *cp;
	register int n;

	for (;;) {
		cp = cbuf;
		do {
			if (cp >= &cbuf[sizeof(cbuf) - 1])
				fatal("Command line too long");
			if ((n = read(1, cp, 1)) != 1) {
				if (n < 0)
					fatal("Lost connection");
				return;
			}
		} while (*cp++ != '\n');
		*--cp = '\0';
		cp = cbuf;
		if (lflag && *cp >= '\1' && *cp <= '\5') {
			printer = NULL;
			log("%s requests %s %s", from, cmdnames[*cp], cp+1);
		}
		switch (*cp++) {
		case '\1':	/* check the queue and print any jobs there */
			printer = cp;
			printjob();
			break;
		case '\2':	/* receive files to be queued */
			printer = cp;
			recvjob();
			break;
		case '\3':	/* display the queue (short form) */
		case '\4':	/* display the queue (long form) */
			printer = cp;
			while (*cp) {
				if (*cp != ' ') {
					cp++;
					continue;
				}
				*cp++ = '\0';
				while (isspace(*cp))
					cp++;
				if (*cp == '\0')
					break;
				if (isdigit(*cp)) {
					if (requests >= MAXREQUESTS)
						fatal("Too many requests");
					requ[requests++] = atoi(cp);
				} else {
					if (users >= MAXUSERS)
						fatal("Too many users");
					user[users++] = cp;
				}
			}
			displayq(cbuf[0] - '\3');
			exit(0);
		case '\5':	/* remove a job from the queue */
			printer = cp;
			while (*cp && *cp != ' ')
				cp++;
			if (!*cp)
				break;
			*cp++ = '\0';
			person = cp;
			while (*cp) {
				if (*cp != ' ') {
					cp++;
					continue;
				}
				*cp++ = '\0';
				while (isspace(*cp))
					cp++;
				if (*cp == '\0')
					break;
				if (isdigit(*cp)) {
					if (requests >= MAXREQUESTS)
						fatal("Too many requests");
					requ[requests++] = atoi(cp);
				} else {
					if (users >= MAXUSERS)
						fatal("Too many users");
					user[users++] = cp;
				}
			}
			rmjob();
			break;
		}
		fatal("Illegal service request");
	}
}

/*
 * Make a pass through the printcap database and start printing any
 * files left from the last time the machine went down.
 */
static
startup()
{
	char buf[BUFSIZ];
	register char *cp;
	int pid;

	printer = buf;

	/*
	 * Restart the daemons.
	 */
	while (getprent(buf) > 0) {
		for (cp = buf; *cp; cp++)
			if (*cp == '|' || *cp == ':') {
				*cp = '\0';
				break;
			}
		if ((pid = fork()) < 0) {
			log("startup: cannot fork");
			cleanup();
		}
		if (!pid) {
			endprent();
			printjob();
		}
	}
}

/*
 * Check to see if the from host has access to the line printer.
 */
static
chkhost(f)
	struct sockaddr_in *f;
{
	register struct hostent *hp;
	register FILE *hostf;
	register char *cp;
	char ahost[50];
	extern char *inet_ntoa();
	char domain[256];

	f->sin_port = ntohs(f->sin_port);
	if (f->sin_family != AF_INET || f->sin_port >= IPPORT_RESERVED)
		fatal("Malformed from address");
	hp = gethostbyaddr(&f->sin_addr, sizeof(struct in_addr), f->sin_family);
	if (hp == 0)
		fatal("Host name for your address (%s) unknown",
			inet_ntoa(f->sin_addr));

	strcpy(fromb, hp->h_name);
	from = fromb;
	if (!strcmp(from, host))
		return;

	getdomainname(domain, sizeof(domain));
	hostf = fopen("/etc/hosts.equiv", "r");
	while (fgets(ahost, sizeof(ahost), hostf)) {
		if (cp = index(ahost, '\n'))
			*cp = '\0';
		cp = index(ahost, ' ');
		if (ahost[0] == '+' && ahost[1] == 0){
			(void) fclose(hostf);
			return;
		}
		else if (ahost[0] == '+' && ahost[1] == '@') {
			if (innetgr(ahost + 2, from, NULL, domain)) {
				(void) fclose(hostf);
				return;
			}
		}
		else if (ahost[0] == '-' && ahost[1] == '@') {
			if (innetgr(ahost + 2, from, 0, domain)) {
			  fatal("Your host does not have line printer access");
			  return;
			}
		}
		else if (ahost[0] == '-') {
			if (!strcmp(from, ahost+1)) {
			  fatal("Your host does not have line printer access");
			  return;
			}
		}
		else if (!strcmp(from, ahost) && cp == NULL) {
			(void) fclose(hostf);
			return;
		}
	}
	fatal("Your host does not have line printer access");
}

/*VARARGS1*/
log(msg, a1, a2, a3)
	char *msg;
{
	short console = isatty(fileno(stderr));

	fprintf(stderr, console ? "\r\n%s: " : "%s: ", name);
	if (printer)
		fprintf(stderr, "%s: ", printer);
	fprintf(stderr, msg, a1, a2, a3);
	if (console)
		putc('\r', stderr);
	putc('\n', stderr);
	fflush(stderr);
}

static
logerr(msg)
	char *msg;
{
	register int err = errno;
	short console = isatty(fileno(stderr));
	extern int sys_nerr;
	extern char *sys_errlist[];

	fprintf(stderr, console ? "\r\n%s: " : "%s: ", name);
	if (msg)
		fprintf(stderr, "%s: ", msg);
	fputs(err < sys_nerr ? sys_errlist[err] : "Unknown error" , stderr);
	if (console)
		putc('\r', stderr);
	putc('\n', stderr);
	fflush(stderr);
}

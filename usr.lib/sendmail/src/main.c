# define  _DEFINE
# include <signal.h>
# include <sys/ioctl.h>
# include "sendmail.h"

SCCSID(@(#)main.c 1.4 87/01/05 SMI); /* from UCB 4.7 12/27/83 */

/*
**  SENDMAIL -- Post mail to a set of destinations.
**
**	This is the basic mail router.  All user mail programs should
**	call this routine to actually deliver mail.  Sendmail in
**	turn calls a bunch of mail servers that do the real work of
**	delivering the mail.
**
**	Sendmail is driven by tables read in from /usr/lib/sendmail.cf
**	(read by readcf.c).  Some more static configuration info,
**	including some code that you may want to tailor for your
**	installation, is in conf.c.  You may also want to touch
**	daemon.c (if you have some other IPC mechanism), acct.c
**	(to change your accounting), names.c (to adjust the name
**	server mechanism).
**
**	Usage:
**		/usr/lib/sendmail [flags] addr ...
**
**		See the associated documentation for details.
**
**	Author:
**		Eric Allman, UCB/INGRES (until 10/81)
**			     Britton-Lee, Inc., purveyors of fine
**				database computers (from 11/81)
**		The support of the INGRES Project and Britton-Lee is
**			gratefully acknowledged.  Britton-Lee in
**			particular had absolutely nothing to gain from
**			my involvement in this project.
*/





int		NextMailer;	/* "free" index into Mailer struct */
char		*FullName;	/* sender's full name */
ENVELOPE	BlankEnvelope;	/* a "blank" envelope */
ENVELOPE	MainEnvelope;	/* the envelope around the basic letter */
ADDRESS		NullAddress = {
	"",	"",	"",	/* q_paddr, q_user, q_host */
};

int		dtablesize = 50;	/* Max file descriptor, plus 1 */
				/* overridden by getdtablesize() under 4.2 */

/*
** Remembered start and end of arg list for setproctitle()
** Be careful to keep these out of BSS by initializing them, or else
** setproctitle() will crash on frozen config files.
*/
static char **Argv = NULL;
static char *LastArgv = NULL;

# ifdef QUEUE
int		OnlyRunId;	/* Queue ID from -M to run, or zero */
char 		*OnlyRunRecip;	/* Recipient name from -R to run, or zero */
# endif QUEUE


#ifdef DAEMON
#ifndef SMTP
ERROR %%%%   Cannot have daemon mode without SMTP   %%%% ERROR
#endif SMTP
#endif DAEMON






main(argc, argv, envp)
	int argc;
	char **argv;
	char **envp;
{
	register char *p;
	char **av, **hv;
	extern int finis();
	extern char Version[];
	char *from;
	typedef int (*fnptr)();
	STAB *st;
	register int i;
	bool readconfig = TRUE;
	bool safecf = TRUE;		/* this conf file is sys default */
	bool queuemode = FALSE;		/* process queue requests */
	static bool reenter = FALSE;
	char jbuf[30];			/* holds HostName */
	extern bool safefile();
	extern time_t convtime();
	extern putheader(), putbody();
	extern ENVELOPE *newenvelope();
	extern intsig();
	extern char **myhostname();
	extern char *arpadate();
	extern char **environ;

#ifdef vax
	/*
	**  Check to see if we reentered.
	**	This would normally happen if e_putheader or e_putbody
	**	were NULL when invoked.
	*/

	if (reenter)
	{
		syserr("main: reentered!");
		abort();
	}
	reenter = TRUE;
#endif vax

	/*
	** Remember the start and extent of argv for setproctitle().
	*/
	Argv = argv;
	LastArgv = argv[argc-1] + strlen(argv[argc-1]);

	setdefaults();
	/*
	**  Do a quick prescan of the argument list.
	**	We do this to find out if we can potentially thaw the
	**	configuration file.  If not, we do the thaw now so that
	**	the argument processing applies to this run rather than
	**	to the run that froze the configuration.
	*/

	argv[argc] = NULL;
	av = argv;
	while ((p = *++av) != NULL)
	{
		if (strncmp(p, "-C", 2) == 0)
		{
			ConfFile = &p[2];
			if (ConfFile[0] == '\0')
				ConfFile = "sendmail.cf";
			safecf = FALSE;
			setgid(getrgid());
			setuid(getruid());
			break;
		}
		else if (strncmp(p, "-bz", 3) == 0)
			break;
	}
	if (p == NULL)
		readconfig = !thaw(FreezeFile);

	/* reset the environment after the thaw */
	/* why??? only BSS should be affected - win */
	environ = envp;

	/*
	**  Now do basic initialization
	*/

	InChannel = stdin;
	OutChannel = stdout;
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGINT, intsig);
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		(void) signal(SIGHUP, intsig);
	(void) signal(SIGTERM, intsig);
	(void) signal(SIGPIPE, SIG_IGN);
	OldUmask = umask(0);
	OpMode = MD_DELIVER;
	MotherPid = getpid();
# ifndef V6
	FullName = getenv("NAME");
# endif V6

	/* set up the blank envelope */
	BlankEnvelope.e_puthdr = putheader;
	BlankEnvelope.e_putbody = putbody;
	BlankEnvelope.e_xfp = NULL;
	BlankEnvelope.e_from = NullAddress;
	CurEnv = &BlankEnvelope;
	MainEnvelope.e_from = NullAddress;

	/*
	**  Be sure we have enough file descriptors.
	*/

	dtablesize = getdtablesize();

	for (i = 3; i < dtablesize; i++)
		(void) close(i);
	errno = 0;

# ifdef LOG
	openlog("sendmail", LOG_PID, 0);
# endif LOG
	errno = 0;
	from = NULL;

	if (readconfig)
	{
		/* initialize some macros, etc. */
		initmacros();

		/* version */
		define('v', Version, CurEnv);
	}

	/* current time */
	define('b', arpadate((char *) NULL), CurEnv);

	/*
	** Crack argv.
	*/

	av = argv;
	p = rindex(*av, '/');
	if (p++ == NULL)
		p = *av;
	if (strcmp(p, "newaliases") == 0)
		OpMode = MD_INITALIAS;
	else if (strcmp(p, "mailq") == 0)
		OpMode = MD_PRINT;
	else if (strcmp(p, "smtpd") == 0)
		OpMode = MD_DAEMON;
	while ((p = *++av) != NULL && p[0] == '-')
	{
		switch (p[1])
		{
		  case 'b':	/* operations mode */
			switch (p[2])
			{
			  case MD_DAEMON:
# ifndef DAEMON
				syserr("Daemon mode not implemented");
				break;
# endif DAEMON
			  case MD_SMTP:
# ifndef SMTP
				syserr("I don't speak SMTP");
				break;
# endif SMTP
			  case MD_ARPAFTP:
			  case MD_DELIVER:
			  case MD_VERIFY:
			  case MD_TEST:
			  case MD_INITALIAS:
			  case MD_PRINT:
			  case MD_FREEZE:
				OpMode = p[2];
				break;

			  default:
				syserr("Invalid operation mode %c", p[2]);
				break;
			}
			break;

		  case 'C':	/* select configuration file (already done) */
			break;
# ifdef DEBUG

		  case 'd':	/* debug */
			tTsetup(tTdvect, sizeof tTdvect, "0-99.1");
			tTflag(&p[2]);
			setbuf(stdout, (char *) NULL);
			printf("Version %s\n", Version);
			break;
# endif DEBUG

		  case 'f':	/* from address */
		  case 'r':	/* obsolete -f flag */
			p += 2;
			if (*p == '\0' && ((p = *++av) == NULL || *p == '-'))
			{
				p = *++av;
				if (p == NULL || *p == '-')
				{
					syserr("No \"from\" person");
					av--;
					break;
				}
			}
			if (from != NULL)
			{
				syserr("More than one \"from\" person");
				break;
			}
			from = newstr(p);
			break;

		  case 'F':	/* set full name */
			p += 2;
			if (*p == '\0' && ((p = *++av) == NULL || *p == '-'))
			{
				syserr("Bad -F flag");
				av--;
				break;
			}
			FullName = newstr(p);
			break;

		  case 'h':	/* hop count */
			p += 2;
			if (*p == '\0' && ((p = *++av) == NULL || !isdigit(*p)))
			{
				syserr("Bad hop count (%s)", p);
				av--;
				break;
			}
			CurEnv->e_hopcount = atoi(p);
			break;
		
		  case 'n':	/* don't alias */
			NoAlias = TRUE;
			break;

		  case 'o':	/* set option */
			setoption(p[2], &p[3], FALSE, TRUE);
			break;

		  case 'q':	/* run queue files at intervals */
# ifdef QUEUE
			queuemode = TRUE;
			QueueIntvl = convtime(&p[2]);
# else QUEUE
			syserr("I don't know about queues");
# endif QUEUE
			break;

		  case 't':	/* read recipients from message */
			GrabTo = TRUE;
			break;

			/* compatibility flags */
		  case 'c':	/* connect to non-local mailers */
		  case 'e':	/* error message disposition */
		  case 'i':	/* don't let dot stop me */
		  case 'm':	/* send to me too */
		  case 'T':	/* set timeout interval */
		  case 'v':	/* give blow-by-blow description */
			setoption(p[1], &p[2], FALSE, TRUE);
			break;

		  case 's':	/* save From lines in headers */
			setoption('f', &p[2], FALSE, TRUE);
			break;

# ifdef DBM
		  case 'I':	/* initialize alias DBM file */
			OpMode = MD_INITALIAS;
			break;
# endif DBM

# ifdef QUEUE
		  case 'M':	/* Queue run takes certain Message-Id only */
			p += 2;
			if (*p == '\0' && ((p = *++av) == NULL || !isdigit(*p)))
			{
				syserr("Bad Message-Id (%s)", p);
				av--;
				break;
			}
			OnlyRunId = atoi(p);
			queuemode = TRUE;
			break;

		  case 'R':	/* Queue run takes certain Recipients only */
			OnlyRunRecip = newstr(p+2);
			queuemode = TRUE;
			break;
# endif QUEUE

		}
	}

	/*
	**  Do basic initialization.
	**	Read system control file.
	**	Extract special fields for local use.
	*/

	if (!safecf || OpMode == MD_FREEZE || readconfig)
		readcf(ConfFile, safecf);

	switch (OpMode)
	{
	  case MD_FREEZE:
		/* this is critical to avoid forgeries of the frozen config */
		setgid(getgid());
		setuid(getuid());

		/* freeze the configuration */
		freeze(FreezeFile);
		exit(EX_OK);

	  case MD_INITALIAS:
		Verbose = TRUE;
		break;
	}

	/* set hostname AFTER freezing */
	hv = myhostname(jbuf, sizeof jbuf);
	if (jbuf[0] != '\0')
	{
		p = newstr(jbuf);
		define('w', p, CurEnv);
		setclass('w', p);
	}
	while (hv != NULL && *hv != NULL)
		setclass('w', *hv++);

	/* do heuristic mode adjustment */
	if (Verbose)
	{
		/* turn off noconnect option */
		setoption('c', "F", TRUE, FALSE);

		/* turn on interactive delivery */
		setoption('d', "", TRUE, FALSE);
	}

	/* our name for SMTP codes */
	expand("\001j", jbuf, &jbuf[sizeof jbuf - 1], CurEnv);
	HostName = jbuf;

	/* the indices of local and program mailers */
	st = stab("local", ST_MAILER, ST_FIND);
	if (st == NULL)
		syserr("No local mailer defined");
	else
		LocalMailer = st->s_mailer;
	st = stab("prog", ST_MAILER, ST_FIND);
	if (st == NULL)
		syserr("No prog mailer defined");
	else
		ProgMailer = st->s_mailer;

	/* operate in queue directory */
	if (chdir(QueueDir) < 0)
	{
		syserr("cannot chdir(%s)", QueueDir);
		exit(EX_SOFTWARE);
	}

	/*
	**  Do operation-mode-dependent initialization.
	*/

	switch (OpMode)
	{
	  case MD_PRINT:
		/* print the queue */
#ifdef QUEUE
		dropenvelope(CurEnv);
		printqueue();
		exit(EX_OK);
#else QUEUE
		usrerr("No queue to print");
		finis();
#endif QUEUE

	  case MD_INITALIAS:
		/* initialize alias database */
		initaliases(AliasFile, TRUE);
		exit(EX_OK);

	  case MD_DAEMON:
		/* don't open alias database -- done in srvrsmtp */
		break;

	  default:
		/* open the alias database */
		initaliases(AliasFile, FALSE);
		break;
	}

# ifdef DEBUG
	if (tTd(0, 15))
	{
		/* print configuration table (or at least part of it) */
		printrules();
		for (i = 0; i < MAXMAILERS; i++)
		{
			register struct mailer *m = Mailer[i];
			int j;

			if (m == NULL)
				continue;
			printf(
  "mailer %d (%s): P=%s s=%d, r=%d, S=%d R=%d M=%ld L=%d F=",
				i, m->m_name, m->m_mailer, 
				m->m_is_rwset, m->m_ir_rwset,
				m->m_os_rwset, m->m_or_rwset,
				m->m_maxsize, m->m_argvsize);
			for (j = '\0'; j <= '\177'; j++)
				if (bitnset(j, m->m_flags))
					putchar(j);
			printf(" E=");
			xputs(m->m_eol);
			printf("\n");
		}
	}
# endif DEBUG

	/*
	**  Switch to the main envelope.
	*/

	MainEnvelope.e_from = NullAddress;
	CurEnv = newenvelope(&MainEnvelope);
	MainEnvelope.e_flags = BlankEnvelope.e_flags;

	/*
	**  If test mode, read addresses from stdin and process.
	*/

	if (OpMode == MD_TEST)
	{
		char buf[MAXLINE];

		printf("ADDRESS TEST MODE\nEnter <ruleset> <address>\n");
		for (;;)
		{
			register char **pvp;
			char *q;
			extern char *DelimChar;

			printf("> ");
			fflush(stdout);
			if (fgets(buf, sizeof buf, stdin) == NULL)
				finis();
			for (p = buf; isspace(*p); *p++)
				;
			q = p;
			while (*p != '\0' && !isspace(*p))
				p++;
			if (*p == '\0')
				continue;
			*p = '\0';
			do
			{
				extern char **prescan();
				char pvpbuf[PSBUFSIZE];

				pvp = prescan(++p, ',', pvpbuf);
				if (pvp == NULL)
					continue;
				rewrite(pvp, 3);
				p = q;
				while (*p != '\0')
				{
					rewrite(pvp, atoi(p));
					while (*p != '\0' && *p++ != ',')
						continue;
				}
			} while (*(p = DelimChar) != '\0');
		}
	}

# ifdef QUEUE
	/*
	**  If collecting stuff from the queue, go start doing that.
	*/

	if (queuemode && OpMode != MD_DAEMON && QueueIntvl == 0)
	{
		runqueue(FALSE);
		finis();
	}
# endif QUEUE

	/*
	**  If a daemon, wait for a request.
	**	getrequests will always return in a child.
	**	If we should also be processing the queue, start
	**		doing it in background.
	**	We check for any errors that might have happened
	**		during startup.
	*/

	if (OpMode == MD_DAEMON || QueueIntvl != 0)
	{
		if (!tTd(0, 1))
		{
			/* put us in background */
			i = fork();
			if (i < 0)
				syserr("daemon: cannot fork");
			if (i != 0)
				exit(0);

			/* get our pid right */
			MotherPid = getpid();

			/* disconnect from our controlling tty */
			disconnect(TRUE);
		}

# ifdef QUEUE
		if (queuemode)
		{
			runqueue(TRUE);
			if (OpMode != MD_DAEMON)
				for (;;)
					pause();
		}
# endif QUEUE
		dropenvelope(CurEnv);

#ifdef DAEMON
		getrequests();

		/* at this point we are in a child: reset state */
		OpMode = MD_SMTP;
		(void) newenvelope(CurEnv);
		openxscript(CurEnv);
#endif DAEMON
	}
	
# ifdef SMTP
	/*
	**  If running SMTP protocol, start collecting and executing
	**  commands.  This will never return.
	*/

	if (OpMode == MD_SMTP)
		smtp();
# endif SMTP

	/*
	**  Do basic system initialization and set the sender
	*/

	initsys();
	setsender(from);

	if (OpMode != MD_ARPAFTP && *av == NULL && !GrabTo)
	{
		CurEnv->e_to = "no recipients specified";
		usrerr("Usage: /usr/lib/sendmail [flags] addr...");
		if (OpMode != MD_VERIFY)
			collect(FALSE);
		finis();
	}
	if (OpMode == MD_VERIFY)
		SendMode = SM_VERIFY;

	/*
	**  Scan argv and deliver the message to everyone.
	*/

	sendtoargv(av);

	/* if we have had errors sofar, arrange a meaningful exit stat */
	if (Errors > 0 && ExitStat == EX_OK)
		ExitStat = EX_USAGE;

	/*
	**  Read the input mail.
	*/

	CurEnv->e_to = NULL;
	if (OpMode != MD_VERIFY || GrabTo)
		collect(FALSE);
	errno = 0;

	/* collect statistics */
	if (OpMode != MD_VERIFY)
		markstats(CurEnv, (ADDRESS *) NULL);

# ifdef DEBUG
	if (tTd(1, 1))
		printf("From person = \"%s\"\n", CurEnv->e_from.q_paddr);
# endif DEBUG

	/*
	**  Actually send everything.
	**	If verifying, just ack.
	*/

	CurEnv->e_from.q_flags |= QDONTSEND;
	CurEnv->e_to = NULL;
	sendall(CurEnv, SM_DEFAULT);

	/*
	** All done.
	*/

	finis();
}
/*
**  FINIS -- Clean up and exit.
**
**	Parameters:
**		none
**
**	Returns:
**		never
**
**	Side Effects:
**		exits sendmail
*/

finis()
{
# ifdef DEBUG
	if (tTd(2, 1))
		printf("\n====finis: stat %d e_flags %o\n", ExitStat, CurEnv->e_flags);
# endif DEBUG

	/* clean up temp files */
	CurEnv->e_to = NULL;
	dropenvelope(CurEnv);

	/* post statistics */
	poststats(StatFile);

	/* and exit */
# ifdef LOG
	if (LogLevel > 11)
		syslog(LOG_DEBUG, "finis, pid=%d", getpid());
# endif LOG
	if (ExitStat == EX_TEMPFAIL)
		ExitStat = EX_OK;
	exit(ExitStat);
}
/*
**  INTSIG -- clean up on interrupt
**
**	This just arranges to exit.  It pessimises in that it
**	may resend a message.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Requeues the current job.
*/

intsig()
{
	register ADDRESS *q;

	FileName = NULL;
	q = CurEnv->e_sendqueue;
	if (q != NULL && !bitset(EF_INQUEUE,CurEnv->e_flags) &&
		OpMode != MD_VERIFY) 
	{
		for (; q != NULL; q = q->q_next)
			if (q != &CurEnv->e_from)
				q->q_flags &= ~QDONTSEND;
		queueup(CurEnv, TRUE, Verbose);
	}
	unlockqueue(CurEnv);
	exit(EX_OK);
}
/*
**  INITMACROS -- initialize the macro system
**
**	This just involves defining some macros that are actually
**	used internally as metasymbols to be themselves.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		initializes several macros to be themselves.
*/

struct metamac
{
	char	metaname;
	char	metaval;
};

struct metamac	MetaMacros[] =
{
	/* LHS pattern matching characters */
	'*', MATCHZANY,	'+', MATCHANY,	'-', MATCHONE,	'=', MATCHCLASS,
	'~', MATCHNCLASS, '%', MATCHYELLOW, '!', MATCHNYELLOW,

	/* these are RHS metasymbols */
	'#', CANONNET,	'@', CANONHOST,	':', CANONUSER,	'>', CALLSUBR,

	/* the conditional operations */
	'?', CONDIF,	'|', CONDELSE,	'.', CONDFI,

	/* and finally the hostname lookup characters */
	'[', HOSTBEGIN,	']', HOSTEND,

	'\0'
};

initmacros()
{
	register struct metamac *m;
	char buf[32];
	register int c;

	for (m = MetaMacros; m->metaname != '\0'; m++)
	{
		buf[0] = m->metaval;
		buf[1] = '\0';
		define(m->metaname, newstr(buf), CurEnv);
	}
	buf[0] = MATCHREPL;
	buf[2] = '\0';
	for (c = '0'; c <= '9'; c++)
	{
		buf[1] = c;
		define(c, newstr(buf), CurEnv);
	}
	getdomainname(buf, sizeof buf);
	define('D', newstr(buf), CurEnv);
	setclass('D', newstr(buf));
}
/*
**  FREEZE -- freeze BSS & allocated memory
**
**	This will be used to efficiently load the configuration file.
**
**	Parameters:
**		freezefile -- the name of the file to freeze to.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Writes BSS and malloc'ed memory to freezefile
*/

union frz
{
	char		frzpad[BUFSIZ];	/* insure we are on a BUFSIZ boundary */
	struct
	{
		time_t	frzstamp;	/* timestamp on this freeze */
		char	*frzbrk;	/* the current break */
		char 	*frzedata;	/* Address of edata */
		char 	*frzend;	/* Address of end */
		char	frzver[252];	/* sendmail version */
	} frzinfo;
};

freeze(freezefile)
	char *freezefile;
{
	int f;
	union frz fhdr;
	extern char edata;
	extern char end;
	extern char *sbrk();
	extern char Version[];

	if (freezefile == NULL)
		return;

	/* try to open the freeze file */
	f = creat(freezefile, FreezeMode);
	if (f < 0)
	{
		syserr("Cannot freeze");
		errno = 0;
		return;
	}

	/* build the freeze header */
	fhdr.frzinfo.frzstamp = curtime();
	fhdr.frzinfo.frzbrk = sbrk(0);
	fhdr.frzinfo.frzedata = &edata;
	fhdr.frzinfo.frzend = &end;
	strcpy(fhdr.frzinfo.frzver, Version);

	/* write out the freeze header */
	if (write(f, (char *) &fhdr, sizeof fhdr) != sizeof fhdr ||
	    write(f, (char *) &edata, (int) (fhdr.frzinfo.frzbrk - &edata)) !=
					(int) (fhdr.frzinfo.frzbrk - &edata))
	{
		syserr("Cannot freeze");
	}

	/* fine, clean up */
	(void) close(f);
}
/*
**  THAW -- read in the frozen configuration file.
**
**	Parameters:
**		freezefile -- the name of the file to thaw from.
**
**	Returns:
**		TRUE if it successfully read the freeze file.
**		FALSE otherwise.
**
**	Side Effects:
**		reads freezefile in to BSS area.
*/

thaw(freezefile)
	char *freezefile;
{
	int f;
	union frz fhdr;
	extern char edata;
	extern char Version[];

	if (freezefile == NULL)
		return (FALSE);

	/* open the freeze file */
	f = open(freezefile, 0);
	if (f < 0)
	{
		errno = 0;
		return (FALSE);
	}

	/* read in the header */
	if (read(f, (char *) &fhdr, sizeof fhdr) < sizeof fhdr ||
	    strcmp(fhdr.frzinfo.frzver, Version) != 0 ||
	    	fhdr.frzinfo.frzedata != &edata ||
	    	fhdr.frzinfo.frzend != &end )
	{
		  /*
		   * Attempt to unlink the out-of-date freeze file
		   */
		write(2, "Freeze file out of date\n", 25);
		(void) unlink(freezefile);
		(void) close(f);
		return (FALSE);
	}

	/* arrange to have enough space */
	if (brk(fhdr.frzinfo.frzbrk) < 0)
	{
		syserr("Cannot break to %x", fhdr.frzinfo.frzbrk);
		(void) close(f);
		return (FALSE);
	}

	/* now read in the freeze file */
	if (read(f, (char *) &edata, (int) (fhdr.frzinfo.frzbrk - &edata)) !=
					(int) (fhdr.frzinfo.frzbrk - &edata))
	{
		/* oops!  we have trashed memory..... */
		write(2, "Cannot read freeze file\n", 24);
		_exit(EX_SOFTWARE);
	}

	(void) close(f);
	return (TRUE);
}
/*
**  DISCONNECT -- remove our connection with any foreground process
**
**	Parameters:
**		fulldrop -- if set, we should also drop the controlling
**			TTY if possible -- this should only be done when
**			setting up the daemon since otherwise UUCP can
**			leave us trying to open a dialin, and we will
**			wait for the carrier.
**
**	Returns:
**		none
**
**	Side Effects:
**		Trys to insure that we are immune to vagaries of
**		the controlling tty.
*/

disconnect(fulldrop)
	bool fulldrop;
{
	int fd;

#ifdef DEBUG
	if (tTd(52, 1))
		printf("disconnect: In %d Out %d\n", fileno(InChannel),
						fileno(OutChannel));
	if (tTd(52, 5))
	{
		printf("don't\n");
		return;
	}
#endif DEBUG

	/* be sure we don't get nasty signals */
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	/* we can't communicate with our caller, so.... */
	HoldErrs = TRUE;
	ErrorMode = EM_MAIL;
	Verbose = FALSE;

	/* all input from /dev/null */
	if (InChannel != stdin)
	{
		(void) fclose(InChannel);
		InChannel = stdin;
	}
	(void) freopen("/dev/null", "r", stdin);

	/* output to the transcript */
	if (OutChannel != stdout)
	{
		(void) fclose(OutChannel);
		OutChannel = stdout;
	}
	if (CurEnv->e_xfp == NULL)
		CurEnv->e_xfp = fopen("/dev/null", "w");
	(void) fflush(stdout);
	(void) close(1);
	(void) close(2);
	if (CurEnv->e_xfp != NULL)
	    while ((fd = dup(fileno(CurEnv->e_xfp))) < 2 && fd > 0)
		continue;

#ifdef TIOCNOTTY
	/* drop our controlling TTY completely if possible */
	if (fulldrop)
	{
		fd = open("/dev/tty", 2);
		if (fd >= 0)
		{
			(void) ioctl(fd, TIOCNOTTY, 0);
			(void) close(fd);
		}
		setpgrp(0);
		errno = 0;
	}
#endif TIOCNOTTY

# ifdef LOG
	if (LogLevel > 11)
		syslog(LOG_DEBUG, "in background, pid=%d", getpid());
# endif LOG

	errno = 0;
}

/*
**  SETPROCTITLE -- set the title of this process for "ps"
**
**	Parameters:
**		format -- a string, possibly containing a print "%"
**			  sequence to format the next parameter, which
**			  becomes the process name shown by "ps" for us.
**		number -- an int which can be formatted by the above.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Clobbers argv[] of our main procedure.
*/
/*VARARGS1*/
setproctitle(format, number)
	char *format;
	int number;
{
	register char *tohere;

	tohere = Argv[0];
	*tohere++ = '-';	/* So ps prints (sendmail)" */
	sprintf(tohere, format, number);
	while (*tohere++) ;		/* Skip to end of printf output */
	while (tohere < LastArgv) *tohere++ = ' ';  /* Avoid confusing ps */
}


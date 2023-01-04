#ifndef lint
static char SccsId[] = "@(#)in.syslog.c 1.1 86/09/25 SMI"; /* from UCB 2.24 06/10/83 */
#endif

/*
**  SYSLOG -- log system messages
**
**	This program implements a system log, implemented as a
**	socket on a 4.2bsd system.  This takes a series of lines.
**	Each line may have a priority, signified as "<n>" as
**	the first three characters of the line.  If this is
**	not present, a default priority (DefPri) is used, which
**	starts out as LOG_ERROR.  The default priority can get
**	changed using "<*>n".
**
**	Timestamps are added if they are not already present.
**
**	To kill syslog, send a signal 15 (terminate).  In 5
**	seconds, syslog will go down.  A signal 1 (hup) will
**	cause it to reread its configuration file.
**
**	Defined Constants:
**		MAXLINE -- the maximimum line length that can be
**			handled.
**		NLOGS -- the maximum number of simultaneous log
**			files.
**		NUSERS -- the maximum number of people that can
**			be designated as "superusers" on your system.
**
**	Author:
**		Eric Allman, UCB/INGRES and Britton-Lee.
*/

# define	DEBUG

# define	NLOGS		10	/* max number of log files */
# define	NSUSERS		10	/* max number of special users */
# define	MAXLINE		256	/* maximum line length */

# define	LOGHOSTNAME	1	/* log hostid on each line */

# define	VFORK()		vfork() /* We have the vfork() sys call */

# include	<syslog.h>
# include	<errno.h>
# include	<stdio.h>
# include	<utmp.h>
# include	<ctype.h>
# include	<sys/param.h>
# include	<sys/stat.h>
# include	<sys/time.h>
# include	<signal.h>
# include	<sysexits.h>
# include	<sys/socket.h>
# include	<netinet/in.h>
# include	<netdb.h>

typedef char	bool;
# define	TRUE		1
# define	FALSE		0

# ifdef DEBUG
# define	dprintf		if (Debug) printf
# else
# define	dprintf		if (0) printf
# endif

# define UNAMESZ	50	/* length of a login name */

/*
**  This structure represents the files that will have log
**  copies printed.
*/

struct filed
{
	char	f_pmask;	/* priority mask */
	bool	f_mark;		/* if set, output timing marks */
	bool	f_tty;		/* if set, this is a tty */
	FILE	*f_file;	/* file descriptor; NULL means unallocated */
	long	f_time;		/* the last time anything was output to it */
	char	f_name[30];	/* filename */
};

struct filed	Files[NLOGS];

/* list of superusers */
char		Susers[NSUSERS][UNAMESZ+1];

int	ShutDown;		/* set if going down */
int	DefPri = LOG_ERROR;	/* the default priority for untagged msgs */
int	Debug;			/* debug flag */
int	LogFile;		/* log mx file descriptor */
int	MarkIntvl = 15;		/* mark interval in minutes */
char	*ConfFile;		/* configuration file */
FILE	*Cons;			/* /dev/console */
int	AnyMarks;		/* there is at least one marked file */

struct timeval tv = { 60, 0 };

struct sockaddr_in	SyslogAddr;

main(argc, argv)
	int argc;
	char **argv;
{
	register int i;
	register char *p;
	extern shutdown();
	extern int domark();
	extern char *LookupHost();
	extern int errno;
	int errct = 0;
	FILE *fp;
	auto int lenSyslogAddr;
	static char *defconf = "/etc/syslog.conf";
	static char *defpid = "/etc/syslog.pid";
	char line[300];

	InitHash();
	while (--argc > 0)
	{
		p = *++argv;
		if (p[0] == '-')
		{
			switch (p[1])
			{
			  case 'm':		/* set mark interval */
				MarkIntvl = atoi(&p[2]);
				if (MarkIntvl <= 0)
					MarkIntvl = 1;
				break;

			  case 'f':		/* configuration file */
				if (p[2] != '\0')
					ConfFile = &p[2];
				else
					ConfFile = defconf;
				break;

			  case 'd':		/* debug */
				Debug++;
				break;
			}
		}
	}

	if (ConfFile == NULL)
		ConfFile = defconf;

	/* try to ignore all signals */
	if (!Debug)
	{
		for (i = 1; i < NSIG; i++)
			signal(i, SIG_IGN);
	}
	else
		signal(SIGINT, shutdown);
	signal(SIGTERM, shutdown);
	signal(SIGALRM, domark);
	alarm(MarkIntvl*60);
	
	/* close all files, except our socket on 0 */
	if (!Debug)
		for (i = 1; i < NOFILE; i++)
			close(i);

	if ((Cons = fopen("/dev/console", "w")) == NULL)
		Cons = fopen("/dev/null", "w");
	setbuf(Cons, NULL);
	LogFile = 0;

	/* now we can run as daemon safely */
	setuid(1);
	dprintf("off & running....\n");
	if (!Debug)
	{
		/* tuck my process id away */
		fp = fopen(defpid, "w");
		if (fp != NULL)
		{
			fprintf(fp, "%d\n", getpid());
			fclose(fp);
		}
	}
	
	init();

	for (;;)
	{
		int in = 1<<LogFile;

		lenSyslogAddr = sizeof SyslogAddr;
		errno = 0;
		i = select(32, &in, 0, 0, AnyMarks ? (struct timeval *)0 : &tv);
		if (i < 0 && errno == EINTR)
			continue;
		if (i <= 0)
			exit(0);
		i = recvfrom(LogFile, line, sizeof line, 0, &SyslogAddr, &lenSyslogAddr);
		if (i < 0)
		{
			if (errno == EINTR)
				continue;
			logerror("read");
			errct++;
			if (errct > 1000)
			{
				logmsg(LOG_SALERT, "syslog: too many errors");
				die();
			}
			sleep(15);
			continue;
		}
		line[i] = '\0';
		printline(line, LookupHost(SyslogAddr.sin_addr));
	}
}
/*
**  LOGERROR -- log an error on the log.
**
**	Parameters:
**		type -- string to print as error type.
**
**	Returns:
**		none.
**
**	Side Effects:
**		outputs the error code in errno to someplace.
*/

logerror(type)
	char *type;
{
	char buf[50];
	extern int errno;

	sprintf(buf, "log %s error %d\n", type, errno);
	errno = 0;
	logmsg(LOG_SALERT, buf);
}

/*
**  PRINTLINE -- print one line
**
**	This is really it -- we have one line -- we crack it and
**	and print it appropriately.
**
**	Parameters:
**		q -- pointer to the line.
**		tag -- the name of the host this is from.
**
**	Returns:
**		none.
**
**	Side Effects:
**		q is broken up and printed.
*/

printline(q, tag)
	register char *q;
	char *tag;
{
	register int i;
	int pri = DefPri;
	char buf[1000];
	char *bp = buf;

	dprintf("message = ``%s''\n", q);
	while (*q != '\0')
	{
		if (bp == buf)
		{
# if LOGHOSTNAME
			if (tag != NULL)
			{
				strcpy(bp, tag);
				strcat(bp, ":\t");
				bp += strlen(bp);
			}
# endif LOGHOSTNAME

			/* test for special codes */
			if (q[0] == '<' && q[2] == '>')
			{
				switch (q[1])
				{
				  case '*':	/* reset default message priority */
					dprintf("default priority = %c\n", q[3]);
					i = q[3] - '0';
					if (i > 0 && i <= 9)
						DefPri = i;
					continue;

				  case '$':	/* reconfigure */
					dprintf("reconfigure\n");
					init();
					continue;
				}
				q++;
				pri = *q++ - '0';
				q++;
				if (pri < 0 || pri > 9)
					pri = DefPri;
			}
		}
		while (*q != '\0' && *q != '\n')
		{
			if (*q != '\r')
				*bp++ = *q;
			q++;
		}
		if (*q == '\0')
			if (bp == buf)
				continue;
			else
				*bp++ = '\n';
		*bp++ = '\0';
		q++;
		bp = buf;

		/* output the line to all files */
		logmsg(pri, bp);
		bp = buf;
	}
}
/*
**  SHUTDOWN -- shutdown the logger
**
**	This should only be done when the system is going down.
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		Starts up an alarm clock, to let other things
**			happen.  Alarm clock will call "die".
**
**	Called By:
**		main
**		signal 15 (terminate)
*/

shutdown()
{
	extern die();

	logmsg(LOG_CRIT, "syslog: shutdown within 5 seconds\n");
	ShutDown++;
	signal(SIGALRM, die);
	alarm(5);
	signal(SIGTERM, die);
	if (Debug)
		signal(SIGINT, die);
}
/*
**  DIE -- really die.
**
**	Parameters:
**		none
**
**	Returns:
**		never
**
**	Side Effects:
**		Syslog dies.
**
**	Requires:
**		exit (sys)
**
**	Called By:
**		alarm clock (signal 14)
*/

die()
{
	alarm(0);
	logmsg(LOG_CRIT, "syslog: down\n");
	sleep(2);	/* wait for output to drain */
	sync();
	exit(0);
}
/*
**  STAMPED -- tell if line is already time stamped.
**
**	Accepts time stamps of the form "Sep 13 00:15:17".
**	Currently just looks for blanks and colons.
**
**	Parameters:
**		l -- the line to check.
**
**	Returns:
**		nonzero -- if the line is time stamped.
**		zero -- otherwise.
**
**	Side Effects:
**		none.
*/

stamped(l)
register char *l;
{
	register int i;

	/* timestamps are at least 15 bytes long */
	for (i = 0; i < 15; i++)
		if (l[i] == '\0')
			return (0);

	/* and they have blanks & colons in well-known places */
	if (l[3] != ' ' || l[6] != ' ' || l[9] != ':' || l[12] != ':')
		return (0);
	return (1);
}
/*
**  LOGMSG -- log a message to the outputs
**
**	Arranges to get the message to the correct places
**	based on the priority of the message.  A timestamp
**	is prepended to the message if one does not already
**	exist.
**
**	Parameters:
**		pri -- the message priority.
**		msg -- the message to be logged.
**
**	Returns:
**		none
**
**	Side Effects:
**		possibly messages to all users, or just specific
**		users.
*/

logmsg(pri, msg)
	int	pri;
	char	*msg;
{
	register char *m;
	register char *p;
	register struct filed *f;
	register int l;
	register int i;
	char buf[MAXLINE+2];
	auto int st;
	auto long now;
	extern char *ctime();
	extern int errno;

	p = buf;
	l = MAXLINE;
	
	/* output a time stamp if one is not already there */
	time(&now);
	if (!stamped(msg))
	{
		m = &ctime(&now)[4];
		for (i = 16; i > 0; i--)
			*p++ = *m++;
		l -= 16;
	}

	/* find the end of the message */
	for (m = msg; *m != '\0' &&l -- >= 0; )
		*p++ = *m++;
	if (*--m != '\n')
		*p++ = '\n';
	
	/* log the message to the particular outputs */
	for (i = 0; i < NLOGS; i++)
	{
		f = &Files[i];
		if (f->f_file == NULL)
			continue;
		if (pri < 0)
		{
			if (!f->f_mark || f->f_time + MarkIntvl*60 > now)
				continue;
		}
		else if (f->f_pmask < pri)
			continue;
		fseek(f->f_file, 0L, 2);
		errno = 0;
		fwrite(buf, p - buf, 1, f->f_file);
		if (f->f_tty)
			fwrite("\r", 1, 1, f->f_file);
		f->f_time = now;
		fflush(f->f_file);
		if (ferror(f->f_file))
		{
			char namebuf[40];

			fclose(f->f_file);
			f->f_file = NULL;
			sprintf(namebuf, "write %s", f->f_name);
			logerror(namebuf);
		}
	}

	/*
	**  Output alert and subalert priority messages to terminals.
	**
	**	We double fork here so that we can continue.  Our
	**	child will fork again and die; we wait for him.
	**	Then process one inherits our grandchildren, we
	**	can run off and have a good time.  Our grandchild
	**	actually tries to do the writing (by calling
	**	wallmsg).
	**
	**	Anything go wrong? -- just give up.
	*/

	if (pri <= LOG_SALERT && pri > 0)
	{
		if (VFORK() == 0)
		{
			if (fork() == 0)
			{
				wallmsg(pri == LOG_ALERT, buf, p - buf);
				_exit(0);
			}
			else
				_exit(0);
		}
		else
			while (wait(&st) >= 0)
				continue;
	}
}
/*
**  INIT -- Initialize syslog from configuration table
**
**	The configuration table consists of a series of lines
**	broken into two sections by a blank line.  The first
**	section gives a list of files to log on.  The first
**	character is a digit which is the priority mask for
**	that file.  If the second digit is an asterisk, then
**	syslog arranges for something to be printed every fifteen
**	minutes (even if only a null line), so that crashes and
**	other events can be localized.  The rest of the line is
**	the pathname of the log file.  The second section is
**	a list of user names; these people are all notified
**	when subalert messages occur (if they are logged on).
**
**	The configuration table will be reread by this routine
**	if a signal 1 occurs; for that reason, it is tricky
**	about not re-opening files and closing files it will
**	not be using.
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		'Files' and 'Susers' are (re)initialized.
*/

init()
{
	register int i;
	register FILE *cf;
	char cline[40+UNAMESZ];
	register struct filed *f;
	register char *p;
	int mark;
	int pmask;
	extern int errno;

	dprintf("init\n");

	/* ignore interrupts during this routine */
	signal(SIGHUP, SIG_IGN);
	logmsg(LOG_INFO, "reinitializing\n");

	/* open the configuration file */
	if ((cf = fopen(ConfFile, "r")) == NULL)
	{
		fprintf(Cons, "\r\nsyslog: cannot open %s (errno %d)\r\n", ConfFile, errno);
		printf("cannot open %s (errno %d)\n", ConfFile, errno);
		return;
	}

	/*
	**  Close all open files.
	*/

	for (f = Files; f < &Files[NLOGS]; f++)
	{
		if (f->f_file != NULL)
			fclose(f->f_file);
		f->f_file = NULL;
	}
	AnyMarks = FALSE;

	/*
	**  Foreach line in the conf table, open that file.
	*/

	f = Files;
	while (fgets(cline, sizeof cline, cf) != NULL)
	{
		dprintf("F: got line >%s<\n", cline, 0);
		/* check for end-of-section */
		if (cline[0] == '\n')
			break;

		/* strip off possible newline character */
		for (p = cline; *p != '\0' && *p != '\n'; p++)
			continue;
		*p = '\0';

		/* extract priority mask and mark flag */
		p = cline;
		mark = FALSE;
		pmask = *p++ - '0';
		if (*p == '*')
		{
			p++;
			mark = TRUE;
		}

		/* insure that it is null-terminated */
		p[sizeof Files[0].f_name - 1] = '\0';

		if (f >= &Files[NLOGS])
			continue;

		/* mark entry as used and update flags */
		strcpy(f->f_name, p);
		f->f_file = fopen(p, "a");
		if (f->f_file == NULL) {
			fprintf(Cons, "\r\nsyslog: cannot open %s (errno %d)\r\n",
				p, errno);
			printf ("syslog: file %s errno %d cannot be opened.\n",
				p, errno);
		} else {
			f->f_time = 0;
			f->f_pmask = pmask;
			f->f_mark = mark;
			f->f_tty = isatty(fileno(f->f_file));
			dprintf("File %s pmask %d mark %d tty %d\n",
				p, pmask, mark, f->f_tty);
			f++;
			if (mark)
				AnyMarks = TRUE;
		}
	}

	/*
	**  Read the list of users.
	**
	**	Anyone in this list is informed directly if s/he
	**	is logged in when a "subalert" or higher priority
	**	message comes through.
	**
	**	Out with the old order, in with the new.
	*/

	for (i = 0; i < NSUSERS && fgets(cline, sizeof cline, cf) != NULL; i++)
	{
		/* strip off newline */
		for (p = cline; *p != '\0' && *p != '\n'; p++)
			continue;
		*p = '\0';
		strcpy(Susers[i], cline);
	}

	/* zero the rest of the old superusers */
	for (; i < NSUSERS; i++)
		Susers[i][0] = '\0';

	/* close the configuration file */
	fclose(cf);

	/* arrange for signal 1 to reconfigure */
	signal(SIGHUP, init);
}
/*
**  WALLMSG -- Write a message to the world at large
**
**	This writes the specified message to either the entire
**	world, or at least a list of approved users.
**
**	It scans the utmp file.  For each user logged in, it
**	checks to see if the user is on the approved list, or if
**	this is an "alert" priority message.  In either case,
**	it opens a line to that typewriter (unless mesg permission
**	is denied) and outputs the message to that terminal.
**
**	Parameters:
**		toall -- if non-zero, writes the message to everyone.
**		msg -- the message to write.
**		len -- the length of the message.
**
**	Returns:
**		none
**
**	Side Effects:
**		none
**
**	Requires:
**		open(sys)
**		read(sys)
**		write(sys)
**		fstat(sys)
**		strcmp(sys)
**		fork(sys)
**		sleep(sys)
**		exit(sys)
**		close(sys)
**
**	Called By:
**		logmsg
*/

wallmsg(toall, msg, len)
	int toall;
	char *msg;
	int len;
{
	struct utmp ut;
	register int i;
	register char *p;
	int uf;
	struct stat statbuf;
	auto long t;
	extern char *ctime();
	char sbuf[1024];
	extern char *gethostname();
	char hbuf[256];

	/* open the user login file */
	uf = open("/etc/utmp", 0);
	if (uf < 0)
		return;

	/* scan the user login file */
	while (read(uf, &ut, sizeof ut) == sizeof ut)
	{
		/* is this slot used? */
		if (ut.ut_name[0] == '\0')
			continue;

		/* if not "alert", check if this user is super */
		if (!toall)
		{
			for (i = 0; i < NSUSERS; i++)
			{
				if (namecheck(Susers[i], ut.ut_name))
					break;
			}
			if (i >= NSUSERS)
			{
				/* nope, just a serf */
				continue;
			}
		}

		/* fork so that the open can't hang us */
		if (fork() != 0)
			continue;
		sleep(1);

		/* compute the device name */
		p = "/dev/12345678";
		strncpy(&p[5], ut.ut_line, 8);

		/* open the terminal */
		i = open(p, 1);
		if (i < 0)
			_exit(1);

		/* does he have write permission? */
		if (fstat(i, &statbuf) < 0 || (statbuf.st_mode & 02) == 0)
		{
			/* no, just pass him by */
			dprintf("Drop user, mode=%o\n", statbuf.st_mode, 0);
			close(i);
			_exit(0);
		}

		/* yes, output the message */
		time(&t);
		strcpy(sbuf, "\r\n\007Broadcast message from ");
		strcat(sbuf, "syslog@");
		gethostname(hbuf, sizeof hbuf);
		strcat(sbuf, hbuf);
		strcat(sbuf, " at ");
		strncat(sbuf, ctime(&t), 24);
		strcat(sbuf, "...\r\n");
		write(i, sbuf, strlen(sbuf));
		p = sbuf;
		while (len-- > 0)
		{
			*msg &= 0177;
			if (*msg == '\n' || *msg == '\t' || *msg == '\r')
				*p++ = *msg++;
			else if (iscntrl(*msg))
			{
				*p++ = '^';
				*p++ = *msg++ ^ 0100;
			}
			else
				*p++ = *msg++;
		}
		strcpy(p, "\r\n");
		write(i, sbuf, strlen(sbuf));

		/* all finished!  go away */
		_exit(0);
	}

	/* close the user login file */
	close(uf);
}
/*
**  CHECKNAME -- Do an equality comparison on names.
**
**	Does right blank padding.
**
**	Parameters:
**		a, b -- pointers to the names to check.
**
**	Returns:
**		1 if equal
**		0 otherwise.
**
**	Side Effects:
**		none
**
**	Requires:
**		none
**
**	Called By:
**		wallmsg
*/

namecheck(a, b)
register char *a, *b;
{
	register int i;

	for (i = 0; i < 8; i++)
	{
		if (*a != *b)
		{
			if (!((*a == ' ' && *b == '\0') || (*a == '\0' && *b == ' ')))
				return (0);
		}
		if (*a != ' ' && *a != '\0')
			a++;
		if (*b != ' ' && *b != '\0')
			b++;
	}
	return (1);
}
/*
**  DOMARK -- Make sure every marked file gets output every 15 minutes
**
**	Just calls "logmsg" with a negative priority every time it
**	gets called.
**
**	Algorithm:
**		create timestamp.
**		call logmsg.
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		sets the alarm clock to call itself after MarkIntvl
**			minutes.
**
**	Requires:
**		logmsg
**
**	Called By:
**		system alarm clock.
**		init
*/

domark()
{
	auto long t;
	extern char *ctime();
	register char *p;
	register char *q;
	char buf[40];

	alarm(0);
	dprintf("domark\n");
	time(&t);
	q = buf;
	for (p = " --- MARK --- "; (*q++ = *p++) != '\0'; )
		continue;
	q--;
	for (p = ctime(&t); (*q++ = *p++) != '\0'; )
		continue;
	logmsg(-1, buf);
	signal(SIGALRM, domark);
	alarm(MarkIntvl*60);
}

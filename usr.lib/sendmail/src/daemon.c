# include <errno.h>
# include "sendmail.h"

#ifndef DAEMON
SCCSID(@(#)daemon.c 1.1 86/09/25 SMI (w/o daemon mode)); /* from UCB 4.11 8/11/84 */
#else

#include <netdb.h>
#include <sys/wait.h>

SCCSID(@(#)daemon.c 1.1 86/09/25 SMI (with daemon mode)); /* from UCB 4.11 8/11/84*/

/*
**  DAEMON.C -- routines to use when running as a daemon.
**
**	This entire file is highly dependent on the 4.2 BSD
**	interprocess communication primitives.  No attempt has
**	been made to make this file portable to Version 7,
**	Version 6, MPX files, etc.  If you should try such a
**	thing yourself, I recommend chucking the entire file
**	and starting from scratch.  Basic semantics are:
**
**	getrequests()
**		Opens a port and initiates a connection.
**		Returns in a child.  Must set InChannel and
**		OutChannel appropriately.
**	clrdaemon()
**		Close any open files associated with getting
**		the connection; this is used when running the queue,
**		etc., to avoid having extra file descriptors during
**		the queue run and to avoid confusing the network
**		code (if it cares).
**	makeconnection(host, port, outfile, infile)
**		Make a connection to the named host on the given
**		port.  Set *outfile and *infile to the files
**		appropriate for communication.  Returns zero on
**		success, else an exit status and errno describing the
**		error.
**	lookuphost(host)
**		Look up the host to determine an address.  Return
**		the address of a struct hostinfo about the host, which
**		may be modified by the caller if desired.
**	closeconnection(fd)
**		Marks the host which we connected to via file desc 'fd'
**		as closed, so we won't try to reuse the connection.
**		Note that this does not actually close the file
**		descriptor.  That's the caller's responsibility.
**
**	The semantics of all of these should be clean.
*/

#define	MAXFD	50		/* # file descs possible */
static struct hostinfo *host_from_fd[MAXFD];	/* Host entry for each */

static jmp_buf NameTimeout;
time_t nametime = 90;		/* seconds to wait for name server */

nametimeout()  {longjmp(NameTimeout, 1);}

/*
**  LOOKUPHOST -- determine host address and other remembered info.
**
**	Parameters:
**		host -- a char * providing a host name or [a.b.c.d].
**
**	Returns:
**		The address of a struct hostinfo for this host.
**		The caller may modify h_fd, h_down, and h_open in
**		this structure.  h_addr and h_exists should not be
**		modified.
**
**	Side Effects:
**		If the hostname has previously been looked up, the
**		existing symbol table entry is returned immediately.
**		Otherwise, a new symbol table entry is created, 
**		a system-dependent hostname->address lookup is done,
**		and the new entry is returned.
**
**		Note that for information to be remembered in the
**		symbol table, the process must do several lookups
**		without forking and exiting.
*/
struct hostinfo *
lookuphost(host)
	char *host;			/* Host name */
{
	STAB	*sp;			/* Symbol table entry */

	sp = stab(host, ST_HOST, ST_ENTER);
	if (!sp->s_value.sv_host.h_valid) {
		/*
		**  Create a new symbol table entry.  Initially it is cleared,
		**  thus h_exists is 0.  Ditto for the other flags.
		**  Then look up the address for the host.
		**  Accept "[a.b.c.d]" syntax for host name.
		*/
		sp->s_value.sv_host.h_valid = 1;
		if (host[0] == '[')
		{
			register char *p = index(host, ']');

			if (p != NULL)
			{
				*p = '\0';
				sp->s_value.sv_host.h_addr.s_addr =
					inet_addr(&host[1]);
				*p = ']';
				if (sp->s_value.sv_host.h_addr.s_addr != -1) {
					sp->s_value.sv_host.h_exists = 1;
				}
			}
		}
		else
		{
 		  register struct hostent *hp;
		  EVENT *ev;

		  if (setjmp(NameTimeout) != 0) {
			sp->s_value.sv_host.h_exists = 1;
			sp->s_value.sv_host.h_down = 1;
			sp->s_value.sv_host.h_errno = ENAMESER;
		  }
		  else {
			ev = setevent(nametime, nametimeout, 0);
			hp = gethostbyname(host);
			if (hp != NULL && hp->h_addrtype == AF_INET) {
				sp->s_value.sv_host.h_addr = 
					*(struct in_addr *)hp->h_addr;
				sp->s_value.sv_host.h_exists = 1;
			}
			clrevent(ev);
		    }
		}
	}

	return &sp->s_value.sv_host;		/* Return old or new entry */
}
/*
**  GETREQUESTS -- open mail IPC port and get requests.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Waits until some interesting activity occurs.  When
**		it does, a child is created to process it, and the
**		parent waits for completion.  Return from this
**		routine is always in the child.  The file pointers
**		"InChannel" and "OutChannel" should be set to point
**		to the communication channel.
*/

extern char *inet_ntoa();

int	DaemonSocket = -1;		/* fd describing socket */
unsigned short	SmtpPort = 0;		/* Cached port number for SMTP */
char	*NetName;			/* name of home (local?) network */

unsigned short
getsmtpport()
{
	register struct servent *sp;

	/*
	**  Set up the address of the SMTP port.
	*/

	return IPPORT_SMTP;
}


getrequests()
{
	int t;
	union wait status;
	struct sockaddr_in	addr;
	int on = 1;

	/*
	**  Try to actually open the connection.
	*/
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = getsmtpport();

	/* get a socket for the SMTP connection */
	DaemonSocket = socket(AF_INET, SOCK_STREAM, 0, 0);
	if (DaemonSocket < 0)
	{
		syserr("getrequests: can't create socket");
	  severe:
# ifdef LOG
		if (LogLevel > 0)
			syslog(LOG_SALERT, "cannot get connection");
# endif LOG
		finis();
	}

#ifdef DEBUG
	/* turn on network debugging? */
	if (tTd(15, 15))
		(void) setsockopt(DaemonSocket, SOL_SOCKET, SO_DEBUG,
				 on, sizeof(on));
#endif DEBUG
	(void) setsockopt(DaemonSocket, SOL_SOCKET, SO_KEEPALIVE,
				 on, sizeof(on));
	(void) setsockopt(DaemonSocket, SOL_SOCKET, SO_REUSEADDR,
				 0, 0);

	if (bind(DaemonSocket, &addr, sizeof addr) < 0)
	{
		/* probably another daemon already */
		syserr("getrequests: cannot bind");
		(void) close(DaemonSocket);
		goto severe;
	}
	listen(DaemonSocket, 10);

# ifdef DEBUG
	if (tTd(15, 1))
		printf("getrequests: %d\n", DaemonSocket);
# endif DEBUG

# ifdef LOG
	/* Tell the log that a new daemon has started. */
	syslog(LOG_INFO, "network daemon starting");
# endif LOG

	for (;;)
	{
		register int pid;
		auto int lotherend;
		struct sockaddr_in otherend;
		extern int RefuseLA;
		extern int QueueLA;

		/* see if we are rejecting connections */
		if (QueueLA > 0 && getla() > QueueLA)
			sleep(5);
		while (RefuseLA > 0 && getla() > RefuseLA)
			sleep(5);

		/* wait for a connection */
		do
		{
			/* pick up old zombies */
			while (wait3(&status, WNOHANG, 0) > 0)
			continue;
			errno = 0;
			lotherend = sizeof otherend;
			t = accept(DaemonSocket, &otherend, &lotherend, 0);
		} while (t < 0 && errno == EINTR);
		if (t < 0)
		{
			syserr("getrequests: accept");
			sleep(5);
			continue;
		}

		/*
		**  Create a subprocess to process the mail.
		*/

# ifdef DEBUG
		if (tTd(15, 2))
			printf("getrequests: forking (fd = %d)\n", t);
# endif DEBUG

		pid = fork();
		if (pid < 0)
		{
			syserr("daemon: cannot fork");
			sleep(10);
			(void) close(t);
			continue;
		}

		if (pid == 0)
		{
			extern struct hostent *gethostbyaddr();
			register struct hostent *hp;
			extern char *RealHostName;	/* srvrsmtp.c */
			char *domain;			/* our domain name */
			char buf[MAXNAME];
			EVENT *ev;

			/*
			**  CHILD -- return to caller.
			**	unbind old YP socket
			**	Collect verified idea of sending host.
			**	Verify calling user id if possible here.
			*/

			(void) close(DaemonSocket);
			if (!yp_get_default_domain(&domain))
				yp_unbind(domain);

			/* determine host name with timeout */
			if (setjmp(NameTimeout) != 0) {
			    hp = NULL;
			}
			else {
			    ev = setevent(nametime, nametimeout, 0);
			    hp = gethostbyaddr(
		&otherend.sin_addr, sizeof otherend.sin_addr, AF_INET);
			    clrevent(ev);
			}
			if (hp != NULL)
				strcpy(buf, hp->h_name);
			else
				strcpy(buf, inet_ntoa(otherend.sin_addr));
			RealHostName = newstr(buf);

			setproctitle("From %s", (int)RealHostName);

			InChannel = fdopen(t, "r");
			OutChannel = fdopen(t, "w");
# ifdef DEBUG
			if (tTd(15, 2))
				printf("getreq: returning\n");
# endif DEBUG
# ifdef LOG
			if (LogLevel > 11)
				syslog(LOG_DEBUG, "connected, pid=%d", getpid());
# endif LOG
			return;
		}

		/*
		**  PARENT -- wait for child to terminate.
		**	Perhaps we should allow concurrent processing?
		*/

# ifdef DEBUG
		if (tTd(15, 2))
		{
			sleep(2);
			printf("getreq: parent waiting\n");
		}
# endif DEBUG

		/* close the port so that others will hang (for a while) */
		(void) close(t);

	}
	/*NOTREACHED*/
}
/*
**  CLRDAEMON -- reset the daemon connection
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		releases any resources used by the passive daemon.
*/

clrdaemon()
{
	if (DaemonSocket >= 0)
		(void) close(DaemonSocket);
	DaemonSocket = -1;
}
/*
**  MAKECONNECTION -- make a connection to an SMTP socket on another machine.
**
**	Parameters:
**		host -- the name of the host.
**		port -- the port number to connect to.
**		outfile -- a pointer to a place to put the outfile
**			descriptor.
**		infile -- ditto for infile.
**
**	Returns:
**		An exit code telling whether the connection could be
**			made and if not why not.
**
**	Side Effects:
**		On an error, global <errno> is set describing the error.
**		A previously existing connection will be reused.
**		Previously cached data will be used to find the host
**			and determine its status.  This cache is updated
**			with the host's current status.
*/

int
makeconnection(host, port, outfile, infile)
	char *host;
	u_short port;
	FILE **outfile;
	FILE **infile;
{
	register struct hostinfo *hp;
	int s;
	struct sockaddr_in	addr;


	/* Remember who we're talking to, for error messages */
	RealHostName = newstr(host);

	/*
	**  Determine the address of the host.
	**  If we have tried to connect before and failed, don't try.
	** Clear errno, since the only relevant errors happen
	** on connection, NOT when we do the name lookup.
	*/

	hp = lookuphost(host);
	errno = 0;
	if (!hp->h_exists)
		return (EX_NOHOST);
	if (hp->h_down)
	{
		if (Verbose) printf("(already known) ");
		errno = hp->h_errno;	/* for a better message */
		return (EX_TEMPFAIL);
	}
	addr.sin_family = AF_INET;
	addr.sin_addr = hp->h_addr;

	/*
	**  Determine the port number.
	*/
	if (port != 0)
		addr.sin_port = htons(port);
	else
		addr.sin_port = getsmtpport();

	if (!hp->h_open || hp->h_port != port)
	{
		s = openhost(hp, addr);
		if (s != EX_OK) return s;
	}
	*outfile = fdopen(hp->h_fd, "w");
	*infile = fdopen(dup(hp->h_fd), "r");
	return (EX_OK);
}

int
openhost(hp, addr)
	struct hostinfo *hp;
	struct sockaddr_in	addr;
{
	register int s;
	int error_code;
	int on = 1;

	/*
	**  Try to actually open the connection.
	*/

# ifdef DEBUG
	if (tTd(16, 1))
		printf("openhost (%x)\n", hp->h_addr.s_addr);
# endif DEBUG

	s = socket(AF_INET, SOCK_STREAM, 0, 0);
	if (s < 0)
	{
		error_code = errno;	/* Save errno for <failure> */
		syserr("openhost: no socket");
		errno = error_code;	/* Save errno for <failure> */
		goto failure;
	}

# ifdef DEBUG
	if (tTd(16, 1))
		printf("openhost: %d\n", s);

	/* turn on network debugging? */
	if (tTd(16, 14))
		(void) setsockopt(s, SOL_SOCKET, SO_DEBUG,
				 on, sizeof(on));
# endif DEBUG
	(void) setsockopt(DaemonSocket, SOL_SOCKET, SO_KEEPALIVE,
				 on, sizeof(on));

	(void) fflush(CurEnv->e_xfp);			/* for debugging */
	errno = 0;					/* for debugging */
	hp->h_tried = 1;		/* We are trying to connect */
	if (connect(s, &addr, sizeof addr, 0) < 0)
	{
		/* failure, decide if temporary or not */
	failure:
		error_code = errno;
		(void) close(s);	/* Free the socket */
		errno = error_code;
		switch (errno)
		{
		  case ETIMEDOUT:
		  	errno = EHOSTDOWN;	/* for a better message */
		  case EISCONN:
		  case EINPROGRESS:
		  case EALREADY:
		  case EADDRINUSE:
		  case EHOSTDOWN:
		  case ENETDOWN:
		  case ENETRESET:
		  case ENOBUFS:
		  case ECONNREFUSED:
		  case ECONNRESET:
		  case EHOSTUNREACH:
		  case ENETUNREACH:
		  case EPERM:
			/* there are others, I'm sure..... */
			hp->h_down = 1;		/* Mark down host */
			hp->h_errno = errno;
			/*
			** Note that the down flag is never turned off.
			** We depend on sendmail's exiting to throw away
			** this information.  It should apply to one
			** queue run only.
			*/
			return (EX_TEMPFAIL);

		  default:
			return (EX_UNAVAILABLE);
		}
	}

	/* connection ok, put it into canonical form */
	host_from_fd[s] = hp;	/* Create cross reference pointer */
	hp->h_port = addr.sin_port;
	hp->h_fd = s;		/* Save file descriptor */
	hp->h_open = 1;		/* And indicate that it's open */

	return (EX_OK);
}
/*
**  CLOSECONNECTION -- mark an open connection as closed.
**
**	Parameters:
**		fd -- the file descriptor of the connection.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Marks the host (which had a connection opened via
**		makeconnection()) as not having a current connection.
**		Note that this does not actually close the file
**		descriptor.  That's the caller's responsibility.
*/
closeconnection(fd)
	int fd;
{
	register struct hostinfo *hp;

	if (fd < MAXFD) {
		hp = host_from_fd[fd];
		if (hp != NULL && hp->h_open) {
			hp->h_open = 0;
		}
		host_from_fd[fd] = NULL;
	}
}
/*
**  MYHOSTNAME -- return the name of this host.
**
**	Parameters:
**		hostbuf -- a place to return the name of this host.
**		size -- the size of hostbuf.
**
**	Returns:
**		A list of aliases for this host.
**
**	Side Effects:
**		none.
*/

char **
myhostname(hostbuf, size)
	char hostbuf[];
	int size;
{
	extern struct hostent *gethostbyname();
	struct hostent *hp;

	gethostname(hostbuf, size);
	hp = gethostbyname(hostbuf);
	if (hp != NULL)
	{
		strncpy(hostbuf, hp->h_name, size-1);
		return (hp->h_aliases);
	}
	else
		return (NULL);
}
/*
**  MAPHOSTNAME -- turn a hostname into canonical form
**
**	Parameters:
**		hbuf -- a buffer containing a hostname.
**		hbsize -- the size of hbuf.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Looks up the host specified in hbuf.  If it is not
**		the canonical name for that host, replace it with
**		the canonical name.  If the name is unknown, or it
**		is already the canonical name, leave it unchanged.
*/

maphostname(hbuf, hbsize)
	char *hbuf;
	int hbsize;
{
	register struct hostent *hp;
	extern struct hostent *gethostbyname();
	int len = strlen(hbuf);

	makelower(hbuf);
	hp = gethostbyname(hbuf);
	if (hp==NULL && strcmp(hbuf+len-5,".arpa")==0) {
		hbuf[len-5] = 0;
		hp = gethostbyname(hbuf);
		if (hp==NULL) hbuf[len-5] = '.';
	}
	if (hp != NULL)
	{
		int i = strlen(hp->h_name);

		if (i >= hbsize)
			hp->h_name[--i] = '\0';
		strcpy(hbuf, hp->h_name);
	}
}

# else DAEMON
/* code for systems without sophisticated networking */

/*
**  MYHOSTNAME -- stub version for case of no daemon code.
**
**	Can't convert to upper case here because might be a UUCP name.
**
**	Mark, you can change this to be anything you want......
*/

char **
myhostname(hostbuf, size)
	char hostbuf[];
	int size;
{
	register FILE *f;

	hostbuf[0] = '\0';
	f = fopen("/usr/include/whoami", "r");
	if (f != NULL)
	{
		(void) fgets(hostbuf, size, f);
		fixcrlf(hostbuf, TRUE);
		(void) fclose(f);
	}
	return (NULL);
}
/*
**  MAPHOSTNAME -- turn a hostname into canonical form
**
**	Parameters:
**		hbuf -- a buffer containing a hostname.
**		hbsize -- the size of hbuf.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Looks up the host specified in hbuf.  If it is not
**		the canonical name for that host, replace it with
**		the canonical name.  If the name is unknown, or it
**		is already the canonical name, leave it unchanged.
*/

/*ARGSUSED*/
maphostname(hbuf, hbsize)
	char *hbuf;
	int hbsize;
{
	return;
}


#endif DAEMON

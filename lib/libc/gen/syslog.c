#ifndef lint
static	char sccsid[] = "@(#)syslog.c 1.1 86/09/24 SMI"; /* from UCB 4.1 83/05/27 */
#endif

/*
 * SYSLOG -- print message on log file
 *
 * This routine looks a lot like printf, except that it
 * outputs to the log file instead of the standard output.
 * Also, it prints the module name in front of lines,
 * and has one other formatting type, %m (error msg from errno).
 * Also, it adds a newline on the end of messages.
 *
 * The output of this routine is intended to be read by
 * /etc/syslog, which will add timestamps.
 *
 * Parameters:
 *	pri -- the message priority.
 *	fmt -- the format string.
 *	p0 -- the first of many parameters.
 *
 * Returns:
 *	none
 *
 * Side Effects:
 *	Opens log if not already open.
 *	output to log.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>

#include <syslog.h>
#include <netdb.h>

#define	MAXLINE	1024		/* max message size */
#define BUFSLOP	20		/* space to allow for "extra stuff" */
#define NULL	0		/* manifest */

static	int	LogFile = -1;		/* fd for log */
static	int	LogStat	= 0;		/* status bits, set by initlog */
static	char	*LogTag = NULL;		/* string to tag the entry with */
static	int	LogMask = LOG_DEBUG;	/* lowest priority to be logged */

static	struct sockaddr_in	SyslogAddr;
static	char	*SyslogHost = LOG_HOST;

extern	int errno, sys_nerr;
extern	char *sys_errlist[];

syslog(pri, fmt, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)
	int	pri;
	char	*fmt;
{
	char	buf[MAXLINE+BUFSLOP], outline[MAXLINE + 1];
	register char *b, *f = fmt, c;
	register int bol, hlen;
	struct iovec iov[MSG_MAXIOVLEN], *iv = iov;

	/* if we have no log, open it */
	if (LogFile < 0)
		(void) openlog(0, 0);

	/* see if we should just throw out this message */
	if (pri > LogMask)
		return;

	b = buf;
	while ((c = *f++) != '\0' && b < buf + MAXLINE) {
		if (c != '%') {
			*b++ = c;
			continue;
		}
		c = *f++;
		if (c != 'm') {
			*b++ = '%', *b++ = c;
			continue;
		}
		if ((unsigned)errno > sys_nerr)
			sprintf(b, "error %d", errno);
		else
			sprintf(b, "%s", sys_errlist[errno]);
		b += strlen(b);
	}
	if (b[-1] != '\n')
		*b++ = '\n';
	*b = '\0';
		
	sprintf(outline, buf, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);

	/*
	 * Build the header.
	 */
	b = buf;
	*b = '\0';
	/* insert priority code */
	if (pri > 0 && (LogStat & LOG_COOLIT) == 0) {
		sprintf(b, "<%d>", pri);
		b += strlen(b);
	}
	/* output current process ID */
	if (LogStat & LOG_PID) {
		sprintf(b, "%d ", getpid());
		b += strlen(b);
	}
	/* and module name */
	if (LogTag)
		sprintf(b, "%s: ", LogTag);
	hlen = strlen(buf);

	/*
	 * Now insert the header in front of every line by using
	 * scatter/gather I/O.
	 */
	iv->iov_base = buf;
	iv->iov_len = hlen;
	iv++;
	iv->iov_base = outline;
	for (bol = 0, b = outline; *b; b++) {
		if (bol) {
			if (iv >= &iov[MSG_MAXIOVLEN-2])
				break;
			iv->iov_len = b - iv->iov_base;
			iv++;
			iv->iov_base = buf;
			iv->iov_len = hlen;
			iv++;
			iv->iov_base = b;
		}
		bol = *b == '\n';
	}
	iv->iov_len = b - iv->iov_base;
	errno = 0;
	if (LogStat & LOG_DGRAM) {
		struct msghdr mh;

		mh.msg_name = (caddr_t)&SyslogAddr;
		mh.msg_namelen = sizeof SyslogAddr;
		mh.msg_iov = iov;
		mh.msg_iovlen = iv - iov + 1;
		mh.msg_accrights = 0;
		mh.msg_accrightslen = 0;
		(void) sendmsg(LogFile, &mh, 0);
	} else {
		(void) writev(LogFile, iov, iv - iov + 1);
		if (errno)
			perror("syslog: sendto");
	}
}

/*
 * OPENLOG -- open system log
 *
 * This happens automatically with reasonable defaults if you
 * do nothing.
 *
 * Parameters:
 *	ident -- the name to be printed as a header for
 *		all messages.  NULL for none.
 *	logstat -- a status word, interpreted as follows:
 *		LOG_PID -- log the pid with each message.
 *
 * Side Effects:
 *	Several global variables get set.
 */
openlog(ident, logstat)
	char *ident;
	int logstat;
{
	struct hostent *hp;

	LogTag = ident;
	LogStat = logstat;
	if (LogFile >= 0)
		return (0);
	hp = gethostbyname(SyslogHost);
	if (hp == NULL)
		goto bad;
	LogFile = socket(hp->h_addrtype, SOCK_DGRAM, 0);
	if (LogFile < 0) {
		perror("syslog: socket");
		goto bad;
	}
	bzero(&SyslogAddr, sizeof SyslogAddr);
	SyslogAddr.sin_family = hp->h_addrtype;
	bcopy(hp->h_addr, (char *)&SyslogAddr.sin_addr, hp->h_length);
	SyslogAddr.sin_port = IPPORT_CMDSERVER;
	LogStat |= LOG_DGRAM;
	ioctl(LogFile, FIOCLEX, NULL);
	return (0);
bad:
	LogStat |= LOG_COOLIT;
	LogStat &= ~LOG_DGRAM;
	LogMask = LOG_CRIT;
	LogFile = open("/dev/console", 1);
	if (LogFile < 0) {
		perror("syslog: /dev/console");
		LogFile = dup(2);
	}

	/* have it close automatically on exec */
	ioctl(LogFile, FIOCLEX, NULL);
	return (-1);
}

/*
  * CLOSELOG -- close the system log
 */
closelog()
{

	(void) close(LogFile);
	LogFile = -1;
}

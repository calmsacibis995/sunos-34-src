#ifndef lint
static	char *sccsid = "@(#)in.tftpd.c 1.1 86/09/25 SMI; from UCB 4.11 ";
#endif

/*
 * Trivial file transfer protocol server.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <netinet/in.h>

#include <arpa/tftp.h>

#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <setjmp.h>

/*
 * Default directory for unqualified names
 * Used by TFTP boot procedures
 */
#define	HOMEDIR	"/tftpboot"

#define	TIMEOUT		5

extern	int errno;
struct	sockaddr_in sin = { AF_INET };
int	peer;
int	rexmtval = TIMEOUT;
int	maxtimeout = 5*TIMEOUT;
char	buf[BUFSIZ];
struct	sockaddr_in from;
int	fromlen;

main()
{
	register struct tftphdr *tp;
	register int n;
	struct servent *sp;

	alarm(10);
	fromlen = sizeof (from);
	n = recvfrom(0, buf, sizeof (buf), 0,
	    (caddr_t)&from, &fromlen);
	if (n < 0)
		exit(1);
	from.sin_family = AF_INET;
	alarm(0);

	/*
	 * We fork here to allow inetd to start other tftpd
	 * processes - we've swallowed the UDP request packet
	 */
	if (fork())
		exit(0);
	close(0);

	peer = socket(AF_INET, SOCK_DGRAM, 0);
	if (peer < 0) {
		perror("tftpd: socket");
		exit(1);
	}
	if (setsockopt(peer, SOL_SOCKET, SO_REUSEADDR, 0, 0) < 0)
		perror("tftpd: setsockopt (SO_REUSEADDR)");
	tp = (struct tftphdr *)buf;
	tp->th_opcode = ntohs(tp->th_opcode);
	if (tp->th_opcode == RRQ || tp->th_opcode == WRQ)
		tftp(tp, n);
}

int	validate_access();
int	sendfile(), recvfile();

struct formats {
	char	*f_mode;
	int	(*f_validate)();
	int	(*f_send)();
	int	(*f_recv)();
} formats[] = {
	{ "netascii",	validate_access,	sendfile,	recvfile },
	{ "octet",	validate_access,	sendfile,	recvfile },
#ifdef notdef
	{ "mail",	validate_user,		sendmail,	recvmail },
#endif
	{ 0 }
};

/*
 * Handle initial connection protocol.
 */
tftp(tp, size)
	struct tftphdr *tp;
	int size;
{
	register char *cp;
	int first = 1, ecode;
	register struct formats *pf;
	char *filename, *mode;

	filename = cp = tp->th_stuff;
again:
	while (cp < buf + size) {
		if (*cp == '\0')
			break;
		cp++;
	}
	if (*cp != '\0') {
		nak(EBADOP);
		exit(1);
	}
	if (first) {
		mode = ++cp;
		first = 0;
		goto again;
	}
	for (cp = mode; *cp; cp++)
		if (isupper(*cp))
			*cp = tolower(*cp);
	for (pf = formats; pf->f_mode; pf++)
		if (strcmp(pf->f_mode, mode) == 0)
			break;
	if (pf->f_mode == 0) {
		nak(EBADOP);
		exit(1);
	}
	/*
	 * Need to perform access check as someone who will only
	 * be allowed "public" access to the file.  There is no
	 * such uid/gid reserved so we kludge it with -2/-2.
	 * (Can't use -1/-1 'cause that means "don't change".)
	 */
	setgid(-2);
	setuid(-2);
	ecode = (*pf->f_validate)(filename, tp->th_opcode);
	if (ecode) {
		/*
		 * The most likely cause of an error here is that
		 * someone has broadcast an RRQ packet because he's
		 * trying to boot and doesn't know who his server his.
		 * Rather then sending an ERROR packet immediately, we
		 * wait a while so that the real server has a better chance
		 * of getting through (in case client has lousy Ethernet
		 * interface).
		 */
		sleep(3);
		nak(ecode);
		exit(1);
	}
	if (tp->th_opcode == WRQ)
		(*pf->f_recv)(pf);
	else
		(*pf->f_send)(pf);
	exit(0);
}

int	fd;

/*
 * Validate file access.  Since we
 * have no uid or gid, for now require
 * file to exist and be publicly
 * readable/writable.
 * Note also, full path name must be
 * given as we have no login directory.
 */
validate_access(file, mode)
	char *file;
	int mode;
{
	struct stat stbuf;

	if (*file != '/')
		if (chdir(HOMEDIR))
			return (EACCESS);
	if (stat(file, &stbuf) < 0)
		return (errno == ENOENT ? ENOTFOUND : EACCESS);
	if (mode == RRQ) {
		if ((stbuf.st_mode&(S_IREAD >> 6)) == 0)
			return (EACCESS);
	} else {
		if ((stbuf.st_mode&(S_IWRITE >> 6)) == 0)
			return (EACCESS);
	}
	if ((stbuf.st_mode & S_IFMT) != S_IFREG)
		return (EACCESS);
	fd = open(file, mode == RRQ ? 0 : 1);
	if (fd < 0)
		return (errno + 100);
	return (0);
}

int	timeout;
jmp_buf	timeoutbuf;

timer()
{

	timeout += rexmtval;
	if (timeout >= maxtimeout)
		exit(1);
	longjmp(timeoutbuf, 1);
}

/*
 * Send the requested file.
 */
sendfile(pf)
	struct format *pf;
{
	struct tftphdr *tp;
	int block = 1, size, n;

	signal(SIGALRM, timer);
	tp = (struct tftphdr *)buf;
	do {
		timeout = 0;
		(void) setjmp(timeoutbuf);
		/* 
		 * We re-enter here on timeouts OR
		 * when the last ACK implies the current block has been dropped.
		 * We must completely reform the DATA packet
		 * since the same buffer is used for incoming packets
		 */
	sendagain:
		lseek(fd, (block-1)*SEGSIZE, 0);
		size = read(fd, tp->th_data, SEGSIZE);
		if (size < 0) {
			nak(errno + 100);
			return;
		}
		tp->th_opcode = htons((u_short)DATA);
		tp->th_block = htons((u_short)block);
		if (sendto(peer, buf, size + 4, 0, (caddr_t)&from, fromlen)
		    != size + 4) {
			perror("tftpd: sendto");
			return;
		}
		do {
			alarm(rexmtval);
			n = recvfrom(peer, buf, sizeof (buf), 0,
			    (caddr_t)&from, &fromlen);
			alarm(0);
			if (n < 4) {
				if (n < 0) {
					perror("tftpd: recvfrom");
				}
				return;
			}
			tp->th_opcode = ntohs((u_short)tp->th_opcode);
			tp->th_block = ntohs((u_short)tp->th_block);
			if (tp->th_opcode == ERROR)
				return;
			if (tp->th_opcode != ACK)
				continue;
			if (tp->th_block == block-1)
				goto sendagain; /* current block was dropped */
		} while (tp->th_block != block);
		block++;
	} while (size == SEGSIZE);
}

/*
 * Receive a file.
 */
recvfile(pf)
	struct format *pf;
{
	struct tftphdr *tp;
	int block = 0, n, size;

	signal(SIGALRM, timer);
	tp = (struct tftphdr *)buf;
	do {
		timeout = 0;
		(void) setjmp(timeoutbuf);
		tp->th_opcode = htons((u_short)ACK);
		tp->th_block = htons((u_short)block);
		if (sendto(peer, buf, 4, 0, (caddr_t)&from, fromlen) != 4) {
			perror("tftpd: sendto");
			goto abort;
		}
		do {
			alarm(rexmtval);
			n = recvfrom(peer, buf, sizeof (buf), 0,
			    (caddr_t)&from, &fromlen);
			alarm(0);
			if (n < 0) {
				perror("tftpd: recvfrom");
				goto abort;
			}
			tp->th_opcode = ntohs((u_short)tp->th_opcode);
			tp->th_block = ntohs((u_short)tp->th_block);
			if (tp->th_opcode == ERROR)
				goto abort;
		} while (tp->th_opcode != DATA || (block+1) != tp->th_block);
		size = write(fd, tp->th_data, n - 4);
		if (size < 0) {
			nak(errno + 100);
			goto abort;
		}
		block++;
	} while (size == SEGSIZE);
abort:
	tp->th_opcode = htons((u_short)ACK);
	tp->th_block = htons((u_short)(block));
	(void) sendto(peer, buf, 4, 0, (caddr_t)&from, fromlen);
}

struct errmsg {
	int	e_code;
	char	*e_msg;
} errmsgs[] = {
	{ EUNDEF,	"Undefined error code" },
	{ ENOTFOUND,	"File not found" },
	{ EACCESS,	"Access violation" },
	{ ENOSPACE,	"Disk full or allocation exceeded" },
	{ EBADOP,	"Illegal TFTP operation" },
	{ EBADID,	"Unknown transfer ID" },
	{ EEXISTS,	"File already exists" },
	{ ENOUSER,	"No such user" },
	{ -1,		0 }
};

/*
 * Send a nak packet (error message).
 * Error code passed in is one of the
 * standard TFTP codes, or a UNIX errno
 * offset by 100.
 */
nak(error)
	int error;
{
	register struct tftphdr *tp;
	int length;
	register struct errmsg *pe;
	extern char *sys_errlist[];

	tp = (struct tftphdr *)buf;
	tp->th_opcode = htons((u_short)ERROR);
	tp->th_code = htons((u_short)error);
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			break;
	if (pe->e_code < 0)
		pe->e_msg = sys_errlist[error - 100];
	strcpy(tp->th_msg, pe->e_msg);
	length = strlen(pe->e_msg);
	tp->th_msg[length] = '\0';
	length += 5;
	if (sendto(peer, buf, length, 0, (caddr_t)&from, fromlen) != length)
		perror("nak");
	exit(1);
}

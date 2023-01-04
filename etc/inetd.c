#ifndef lint
static	char sccsid[] = "@(#)inetd.c 1.1 86/09/24 Copyr 1985 Sun Micro"; /* from UCB 4.24 83/07/01 */
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * inetd - Internet super-server
 *
 * this program invokes all internet services as needed.
 * connection-oriented services are invoked each time a
 * connection is made, by creating a process.  this process
 * is passed the connection as file descriptor 0 and an
 * argument of the form
 *	sourcehost.sourceport
 * where sourcehost is hex and sourceport is decimal
 *
 * datagram oriented services are invoked when a datagram
 * arrives; a process is created and passed the connection
 * as file descriptor 0.  inetd will look at the socket
 * where datagrams arrive again only after this process
 * completes.  the paradigms for such processes is to read
 * off the incoming datagram and then fork and exit, or
 * to process the arriving datagrams and time out.
 */
#define SERVTAB "/etc/servers"
#define streq(a,b) (strcmp(a,b) == 0)

#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/wait.h>

#include <arpa/inet.h>

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <netdb.h>

extern	int errno;

int	reapchild();
char	*index(), *rindex();

int	debug;
struct	servent *sp;

#define KIND_UDP 1
#define KIND_TCP 2
#define KIND_RPC_UDP 3
#define KIND_RPC_TCP 4

struct	servtab {
	char	*se_service;
	int	se_kind;
	int	se_pid;
	char	*se_server;
	int	se_fd;
	union {
		struct	sockaddr_in ctrladdr;
		struct{
			unsigned prog;
			unsigned lowvers;
			unsigned highvers;
		} rpcnum
	} se_un;
	struct	servtab *se_nxt;
} *servtab;

#define se_ctrladdr se_un.ctrladdr 
#define se_rpc se_un.rpcnum

int	allsock;

main(argc, argv)
	int argc;
	char *argv[];
{
	int ctrl, options = 0;
	register struct servtab *sep;
	char *cp;
	int pid;

	debug = 0;
	argc--, argv++;
	while (argc > 0 && *argv[0] == '-') {
		for (cp = &argv[0][1]; *cp; cp++) switch (*cp) {

		case 'd':
			debug = 1;
			options |= SO_DEBUG;
			break;

		default:
			fprintf(stderr,
			    "inetd: Unknown flag -%c ignored.\n", *cp);
			break;
		}
nextopt:
		argc--, argv++;
	}
	readservtab ();
#ifndef DEBUG
	if (fork())
		exit(0);
	{ int s;
	for (s = 0; s < 10; s++)
		(void) close(s);
	}
	(void) open("/", O_RDONLY);
	(void) dup2(0, 1);
	(void) dup2(0, 2);
#endif
	{ int tt = open("/dev/tty", O_RDWR);
	  if (tt > 0) {
		ioctl(tt, TIOCNOTTY, 0);
		(void)close(tt);
	  }
	}
	for (sep = servtab; sep; sep = sep->se_nxt) {
		sep->se_fd = -1;
		if (sep->se_kind == KIND_RPC_UDP
		    || sep->se_kind == KIND_RPC_TCP) {
			if ((sep->se_fd = getrpcsock(sep->se_rpc.prog,
			    sep->se_rpc.lowvers, sep->se_rpc.highvers,
			    sep->se_kind)) < 0) {
				perror("getrpcsock");
				continue;
			    }
			allsock |= (1 << sep->se_fd);
#ifdef DEBUG
			printf("registered %s\n", sep->se_service);
#endif
			continue;
		}
		sp = getservbyname(sep->se_service,
		    (sep->se_kind == KIND_UDP) ? "udp" : "tcp");
		if (sp == 0) {
			fprintf(stderr,
			    "inetd: %s/tcp: unknown service\n",
			    sep->se_service);
			continue;
		}
		sep->se_ctrladdr.sin_family = AF_INET;
		sep->se_ctrladdr.sin_addr.s_addr = 0;
		sep->se_ctrladdr.sin_port = sp->s_port;
		if ((sep->se_fd = socket(AF_INET,
		    (sep->se_kind == KIND_UDP) ? SOCK_DGRAM : SOCK_STREAM,
		    0)) < 0) {
#ifdef DEBUG
			fprintf(stderr, "inetd: %s/%s: ", sep->se_service,
			    (sep->se_kind == KIND_UDP) ? "udp" : "tcp");
			perror("socket");
#endif
			continue;
		}
		if (sep->se_kind == KIND_TCP && (options & SO_DEBUG) &&
		    setsockopt(sep->se_fd, SOL_SOCKET, SO_DEBUG, 0, 0) < 0) {
#ifdef DEBUG
			perror("inetd: setsockopt (SO_DEBUG)");
#endif
		}
		if (setsockopt(sep->se_fd, SOL_SOCKET, SO_REUSEADDR,
		    0, 0) < 0) {
#ifdef DEBUG
			perror("inetd: setsockopt (SO_REUSEADDR)");
#endif
		}
		if (bind(sep->se_fd, &sep->se_ctrladdr,
		    sizeof (sep->se_ctrladdr), 0) < 0) {
#ifdef DEBUG
			fprintf(stderr, "inetd: %s/%s: ", sep->se_service,
			    (sep->se_kind == KIND_UDP) ? "udp" : "tcp");
			perror("bind");
#endif
			continue;
		}
		if (sep->se_kind == KIND_TCP)
			listen(sep->se_fd, 10);
		allsock |= 1<<sep->se_fd;
	}
	signal(SIGCHLD, reapchild);
	if (allsock)
	for (;;) {
		int readable = allsock;
		int s, ctrl;
		struct sockaddr_in his_addr;
		int hisaddrlen = sizeof (his_addr);

		if (select(32, &readable, 0, 0, 0) <= 0)
			continue;
		s = ffs(readable)-1;
		if (s < 0)
			continue;
		for (sep = servtab; sep; sep = sep->se_nxt)
			if (s == sep->se_fd)
				goto found;
		abort(1);
	found:
#ifdef DEBUG
		fprintf(stderr, "someone wants %s (%d)\n", sep->se_service,
		    sep->se_kind);
#endif
		if (sep->se_kind == KIND_TCP) {
			ctrl = accept(s, &his_addr, &hisaddrlen);
#ifdef DEBUG
			fprintf(stderr, "accept, ctrl %d\n", ctrl);
#endif
			if (ctrl < 0) {
				if (errno == EINTR)
					continue;
				perror("inetd: accept");
				continue;
			}
		} else
			ctrl = sep->se_fd;
		sigblock(1<<(SIGCHLD-1));
		pid = fork();
		if (pid < 0) {
			if (sep->se_kind == KIND_TCP)
				(void)close(ctrl);
			sleep(1);
			continue;
		}
		if (sep->se_kind != KIND_TCP) {
			sep->se_pid = pid;
			allsock &= ~(1<<s);
		}
		sigsetmask(0);
		if (pid == 0) {
			char addrbuf[32];
			int i;

			sprintf(addrbuf, "%x.%d",
			    ntohl(his_addr.sin_addr.s_addr),
			    ntohs(his_addr.sin_port));
			dup2(ctrl, 0), (void)close(ctrl), dup2(0, 1);
			for (i = getdtablesize(); --i > 2; )
				(void)close(i);
#ifdef DEBUG
			fprintf(stderr, "%d execl %s\n", getpid(),
			    sep->se_server);
#endif
			execl(sep->se_server, rindex(sep->se_server, '/')+1,
			    (sep->se_kind == KIND_UDP
			    || sep->se_kind == KIND_RPC_UDP)
			    ? (char *)0 : addrbuf, (char *)0);
			/* 
			 * if exec fails, do some cleanup
			 */
			if (sep->se_kind == KIND_UDP
			    || sep->se_kind == KIND_RPC_UDP)
				recv(0, addrbuf, sizeof (addrbuf));
			if (sep->se_kind == KIND_RPC_TCP)
				(void)close(accept(0, &his_addr, &hisaddrlen));
#ifdef DEBUG
			fprintf(stderr, "execl failed\n");
#endif
			_exit(1);
		}
		if (sep->se_kind == KIND_TCP)
			(void)close(ctrl);
	}
}

reapchild()
{
	union wait status;
	int pid;
	register struct servtab *sep;

	for (;;) {
		pid = wait3(&status, WNOHANG, 0);
		if (pid <= 0)
			return;
#ifdef DEBUG
		fprintf(stderr, "%d reaped\n", pid);
#endif
		for (sep = servtab; sep; sep = sep->se_nxt)
			if (sep->se_pid == pid) {
#ifdef DEBUG
			fprintf(stderr, "restored %s, fd %d\n",
			    sep->se_service, sep->se_fd);
#endif
/* should have saved a copy of the datagram which caused this */
/* server to be invoked, and now get a copy of the first datagram */
/* waiting on this socket, if any, and compare the two.  if they */
/* are the same we should discard this first datagram. */
			allsock |= 1<<sep->se_fd;
		}
	}
}

char *skip();

readservtab ()
{
	FILE *fp;
	char line[BUFSIZ], *p, *service, *udp;
        char *server, *cp, *any();
	int     kind, prognum, lowvers, highvers;
	struct servtab *svtab;

	fp = fopen(SERVTAB, "r");
	if (fp == NULL) {
		fprintf(stderr, "can't read %s\n", SERVTAB);
		exit(1);
	}
	while (1) {
		if ((p = fgets(line, BUFSIZ, fp)) == NULL) {
			fclose(fp);
			return;
		}
		if (*p == '#')
			continue;
		cp = any(p, "#\n");
		if (cp == NULL)
			continue;
		*cp = '\n';
		while (*p == ' ' || *p == '\t')
			p++;
		service = p;
		if ((p = skip(p)) == NULL)
			continue;
		udp = p;
		if ((p = skip(p)) == NULL)
			continue;
		server = p;
		if (streq(udp, "udp"))
			 kind = KIND_UDP;
		else if (streq(udp, "tcp"))
			kind = KIND_TCP;
		else
			continue;
		cp = NULL;
		if (streq(service, "rpc")) {
			if (kind == KIND_UDP)
				kind = KIND_RPC_UDP;
			else
				kind = KIND_RPC_TCP;
			if ((p = skip(p)) == NULL)
				continue;
			prognum = atoi(p);
			if ((p = skip(p)) == NULL)
				continue;
			lowvers = atoi(p);
			cp = index(p, '-');
			if (cp != NULL)
				highvers = atoi(cp+1);
			else
				highvers = lowvers;
		}
		p = any(p, " \t\n");
		*p = '\0';
		svtab = (struct servtab *)malloc(sizeof(struct servtab));
		svtab->se_service = (char *)malloc(strlen(service) + 1);
		strcpy (svtab->se_service, service);
		svtab->se_kind = kind;
		svtab->se_server = (char *)malloc(strlen(server) + 1);
		strcpy(svtab->se_server, server);
		svtab->se_fd = -1;
		svtab->se_rpc.prog = prognum;
		svtab->se_rpc.lowvers = lowvers;
		svtab->se_rpc.highvers = highvers;
		svtab->se_nxt = servtab;
		servtab = svtab;
	}
}

/* 
 * scans cp, looking for a match with any character
 * in match.  Returns pointer to place in cp that matched
 * (or NULL if no match)
 */
static char *
any(cp, match)
	register char *cp;
	char *match;
{
	register char *mp, c;

	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return ((char *)0);
}

/* 
 *  Skip over current field, mark end of it with \0, and return
 *  pointing to beginning of next field
 */
char *
skip(p)
	char *p;
{
	p = any(p, " \t");
	if (p == NULL)
		return NULL;
	*p++ = '\0';
	while (*p == ' ' || *p == '\t')
		p++;
	return p;
}

getrpcsock(prognum, lowvers, highvers, kind)
{
	int s, i;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);
	static int first = 1;
	static int ok = 1;
	
	/* 
	 * first time thru, do all unsets.  Have to do this here
	 * because there might be multiple registers of the same prognum
	 */
	if (first) {
		register struct servtab *sep;

		for (sep = servtab; sep; sep = sep->se_nxt) {
			if (sep->se_kind == KIND_RPC_UDP
			    || sep->se_kind == KIND_RPC_TCP) {
				for (i = sep->se_rpc.lowvers;
				    i <= sep->se_rpc.highvers; i++)
					pmap_unset(sep->se_rpc.prog, i);
			}
		}
		first = 0;
	}
	if ((s = socket(AF_INET,
	    (kind == KIND_RPC_UDP) ? SOCK_DGRAM : SOCK_STREAM, 0)) < 0) {
		perror("inet: socket");
		return -1;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	addr.sin_addr.s_addr = 0;
	if (bind(s, &addr, sizeof(addr)) < 0) {
		perror("bind");
		(void)close(s);
		return -1;
	}
	if (getsockname(s, &addr, &len) != 0) {
		perror("inet: getsockname");
		(void)close(s);
		return -1;
	}
	for (i = lowvers; i <= highvers; i++) {
                pmap_set(prognum, i,
		    (kind == KIND_RPC_UDP) ? IPPROTO_UDP : IPPROTO_TCP,
		    htons(addr.sin_port));
	}
	if (kind == KIND_RPC_TCP)
		listen(s, 10);
	return s;
}

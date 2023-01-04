#ifndef lint
static	char sccsid[] = "@(#)dumprmt.c 1.1 86/09/24 SMI"; /* from UCB 1.6 83/07/01 */
#endif

#include <sys/param.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>

#include <netinet/in.h>

#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <netdb.h>

#define	TS_CLOSED	0
#define	TS_OPEN		1

static	int rmtstate = TS_CLOSED;
int	rmtape;
int	rmtconnaborted();
char	*rmtpeer;

extern	char *rindex();

rmthost(host)
	char *host;
{

	rmtpeer = host;
	signal(SIGPIPE, rmtconnaborted);
	rmtgetconn();
	if (rmtape < 0)
		exit(1);
}

rmtconnaborted()
{

	fprintf(stderr, "Lost connection to remote host.\n");
	exit(1);
}

rmtgetconn()
{
	static struct servent *sp = 0;
	static struct passwd *pwd = 0;
	char *tuser;

	if (sp == 0) {
		sp = getservbyname("shell", "tcp");
		if (sp == 0) {
			fprintf(stderr, "rdump: shell/tcp: unknown service\n");
			exit(1);
		}
		pwd = getpwuid(getuid());
		if (pwd == 0) {
			fprintf(stderr, "rdump: who are you?\n");
			exit(1);
		}
	}
	tuser = rindex(rmtpeer, '.');
	if (tuser) {
		*tuser++ = 0;
		if (!okname(tuser))
			exit(1);
	} else
		tuser = pwd->pw_name;
	rmtape = rcmd(&rmtpeer, sp->s_port, pwd->pw_name, tuser, "/etc/rmt", 0);
}

okname(cp0)
	char *cp0;
{
	register char *cp;
	register int c;

	for (cp = cp0; *cp; cp++) {
		c = *cp;
		if (!isascii(c) || !(isalnum(c) || c == '_' || c == '-')) {
			fprintf(stderr, "rdump: invalid user name %s\n", cp0);
			return (0);
		}
	}
	return (1);
}

rmtopen(tape, mode)
	char *tape;
	int mode;
{
	char buf[256];

	sprintf(buf, "O%s\n%d\n", tape, mode);
	rmtcall(tape, buf);
	rmtstate = TS_OPEN;
}

rmtclose()
{

	if (rmtstate != TS_OPEN)
		return;
	rmtcall("close", "C\n");
	rmtstate = TS_CLOSED;
}

rmtread(buf, count)
	char *buf;
	int count;
{
	char line[30];
	int n, i, cc;
	extern errno;

	sprintf(line, "R%d\n", count);
	n = rmtcall("read", line);
	if (n < 0) {
		errno = n;
		return (-1);
	}
	for (i = 0; i < n; i += cc) {
		cc = read(rmtape, buf+i, n - i);
		if (cc <= 0) {
			rmtconnaborted();
		}
	}
	return (n);
}

rmtwrite(buf, count)
	char *buf;
	int count;
{
	char line[30];

	sprintf(line, "W%d\n", count);
	write(rmtape, line, strlen(line));
	write(rmtape, buf, count);
	return (rmtreply("write"));
}

rmtwrite0(count)
	int count;
{
	char line[30];

	sprintf(line, "W%d\n", count);
	write(rmtape, line, strlen(line));
}

rmtwrite1(buf, count)
	char *buf;
	int count;
{

	write(rmtape, buf, count);
}

rmtwrite2()
{
	int i;

	return (rmtreply("write"));
}

rmtseek(offset, pos)
	int offset, pos;
{
	char line[80];

	sprintf(line, "L%d\n%d\n", offset, pos);
	return (rmtcall("seek", line));
}

struct	mtget mts;

struct mtget *
rmtstatus()
{
	register int i;
	register char *cp;

	if (rmtstate != TS_OPEN)
		return (0);
	rmtcall("status", "S\n");
	for (i = 0, cp = (char *)&mts; i < sizeof(mts); i++)
		*cp++ = rmtgetb();
	return (&mts);
}

rmtioctl(cmd, count)
	int cmd, count;
{
	char buf[256];

	if (count < 0)
		return (-1);
	sprintf(buf, "I%d\n%d\n", cmd, count);
	return (rmtcall("ioctl", buf));
}

rmtcall(cmd, buf)
	char *cmd, *buf;
{

	if (write(rmtape, buf, strlen(buf)) != strlen(buf))
		rmtconnaborted();
	return (rmtreply(cmd));
}

rmtreply(cmd)
	char *cmd;
{
	register int c;
	char code[30], emsg[BUFSIZ];

	rmtgets(code, sizeof (code));
	if (*code == 'E' || *code == 'F') {
		rmtgets(emsg, sizeof (emsg));
		msg("%s: %s\n", cmd, emsg, code + 1);
		if (*code == 'F') {
			rmtstate = TS_CLOSED;
			return (-1);
		}
		return (-1);
	}
	if (*code != 'A') {
		msg("Protocol to remote tape server botched (code %s?).\n",
		    code);
		rmtconnaborted();
	}
	return (atoi(code + 1));
}

rmtgetb()
{
	char c;

	if (read(rmtape, &c, 1) != 1)
		rmtconnaborted();
	return (c);
}

rmtgets(cp, len)
	char *cp;
	int len;
{

	while (len > 1) {
		*cp = rmtgetb();
		if (*cp == '\n') {
			cp[1] = 0;
			return;
		}
		cp++;
		len--;
	}
	msg("Protocol to remote tape server botched (in rmtgets).\n");
	rmtconnaborted();
}

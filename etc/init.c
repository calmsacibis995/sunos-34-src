#ifndef lint
static	char *sccsid = "@(#)init.c 1.1 86/09/24 SMI"; /* from UCB 4.12 */
#endif

#include <signal.h>
#include <sys/types.h>
#include <utmp.h>
#include <setjmp.h>
#include <sys/reboot.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#define	LINSIZ	sizeof(wtmp.ut_line)
#define	TABSIZ	100
#define	ALL	p = &itab[0]; p < &itab[TABSIZ]; p++
#define	EVER	;;
#define SCPYN(a, b)	strncpy(a, b, sizeof(a))
#define SCMPN(a, b)	strncmp(a, b, sizeof(a))
#define	mask(s)	(1 << ((s)-1))

char	shell[]	= "/bin/sh";
char	getty[]	 = "/etc/getty";
char	minus[]	= "-";
char	bootc[]	= "/etc/rc.boot";
char	runc[]	= "/etc/rc";
char	ifile[]	= "/etc/ttys";
char	utmp[]	= "/etc/utmp";
char	wtmpf[]	= "/usr/adm/wtmp";
char	ctty[]	= "/dev/console";
char	dev[]	= "/dev/";

struct utmp wtmp;
struct
{
	char	line[LINSIZ];
	char	comn;
	char	flag;
} line;
struct	tab
{
	char	line[LINSIZ];
	char	comn;
	char	xflag;
	int	pid;
	time_t	gettytime;
	int	gettycnt;
} itab[TABSIZ];

int	fi;
int	mergflag;
char	tty[20];
jmp_buf	sjbuf, shutpass;
time_t	time0;

int	reset();
int	idle();
char	*strcpy(), *strcat();
long	lseek();

struct	sigvec rvec = { reset, mask(SIGHUP), 0 };

#ifdef vax
main()
{
	register int r11;		/* passed thru from boot */
#else
main(argc, argv)
	char **argv;
{
#endif
	int howto, oldhowto;

	time0 = time(0);
#ifdef vax
	howto = r11;
#else
	if (argc > 1 && argv[1][0] == '-') {
		char *cp;

		howto = 0;
		cp = &argv[1][1];
		while (*cp) switch (*cp++) {
		case 'a':
			howto |= RB_ASKNAME;
			break;
		case 's':
			howto |= RB_SINGLE;
			break;
		case 'b':
			howto |= RB_NOBOOTRC;
			break;
		}
	} else {
		howto = RB_SINGLE;
	}
#endif
	sigvec(SIGTERM, &rvec, (struct sigvec *)0);
	signal(SIGTSTP, idle);
	signal(SIGSTOP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	(void) setjmp(sjbuf);
	if ((howto&RB_NOBOOTRC) == 0 && access(bootc, 4) == 0) {
		howto |= RB_NOBOOTRC;
		if (bootrc(howto) == 0)
			/* if rc.boot exits abnormally, stay single-user */
			howto = RB_SINGLE|RB_NOBOOTRC;
	}
	for (EVER) {
		oldhowto = howto;
		howto = RB_SINGLE|RB_NOBOOTRC;
		if (setjmp(shutpass) == 0)
			shutdown();
		if (oldhowto & RB_SINGLE)
			single();
		if (runcom(oldhowto) == 0) 
			continue;
		merge();
		multiple();
	}
}

int	shutreset();

shutdown()
{
	register i;
	register struct tab *p;

	close(creat(utmp, 0644));
	signal(SIGHUP, SIG_IGN);
	for (ALL) {
		term(p);
		p->line[0] = 0;
	}
	signal(SIGALRM, shutreset);
	alarm(30);
	for (i = 0; i < 5; i++)
		kill(-1, SIGKILL);
	while (wait((int *)0) != -1)
		;
	alarm(0);
	shutend();
}

char shutfailm[] = "WARNING: Something is hung (wont die); ps axl advised\n";

shutreset()
{
	int status;

	if (fork() == 0) {
		int ct = open(ctty, 1);
		write(ct, shutfailm, sizeof (shutfailm));
		sleep(5);
		exit(1);
	}
	sleep(5);
	shutend();
	longjmp(shutpass, 1);
}

shutend()
{
	register i, f;

	acct(0);
	signal(SIGALRM, SIG_DFL);
	for (i = 0; i < 10; i++)
		close(i);
	f = open(wtmpf, O_WRONLY|O_APPEND);
	if (f >= 0) {
		SCPYN(wtmp.ut_line, "~");
		SCPYN(wtmp.ut_name, "shutdown");
		SCPYN(wtmp.ut_host, "");
		time(&wtmp.ut_time);
		write(f, (char *)&wtmp, sizeof(wtmp));
		close(f);
	}
	return (1);
}

bootrc(howto)
	int howto;
{
	register int pid;
	int status;

	pid = fork();
	if (pid == 0) {
		(void) open("/", O_RDONLY);
		dup2(0, 1);
		dup2(0, 2);
		if (howto & RB_SINGLE)
			execl(shell, shell, bootc, "singleuser", (char *)0);
		else
			execl(shell, shell, bootc, (char *)0);
		exit(1);
	}
	while (wait(&status) != pid)
		;
	if (status)
		return (0);
	else
		return (1);
}

single()
{
	register pid;
	register xpid;
	extern	errno;

	do {
		pid = fork();
		if (pid == 0) {
			signal(SIGTERM, SIG_DFL);
			signal(SIGHUP, SIG_DFL);
			signal(SIGALRM, SIG_DFL);
			signal(SIGTSTP, SIG_IGN);
			signal(SIGTTIN, SIG_IGN);
			signal(SIGTTOU, SIG_IGN);
			(void) open(ctty, O_RDWR);
			dup2(0, 1);
			dup2(0, 2);
			execl(shell, minus, (char *)0);
			exit(0);
		}
		while ((xpid = wait((int *)0)) != pid)
			if (xpid == -1 && errno == ECHILD)
				break;
	} while (xpid == -1);
}

runcom(oldhowto)
	int oldhowto;
{
	register pid, f;
	int status;

	pid = fork();
	if (pid == 0) {
		(void) open("/", O_RDONLY);
		dup2(0, 1);
		dup2(0, 2);
		if (oldhowto & RB_SINGLE)
			execl(shell, shell, runc, (char *)0);
		else
			execl(shell, shell, runc, "autoboot", (char *)0);
		exit(1);
	}
	while (wait(&status) != pid)
		;
	if (status)
		return (0);
	f = open(wtmpf, O_WRONLY|O_APPEND);
	if (f >= 0) {
		SCPYN(wtmp.ut_line, "~");
		SCPYN(wtmp.ut_name, "reboot");
		SCPYN(wtmp.ut_host, "");
		if (time0) {
			wtmp.ut_time = time0;
			time0 = 0;
		} else
			time(&wtmp.ut_time);
		write(f, (char *)&wtmp, sizeof(wtmp));
		close(f);
	}
	return (1);
}

struct	sigvec	mvec = { merge, mask(SIGTERM), 0 };
/*
 * Multi-user.  Listen for users leaving, SIGHUP's
 * which indicate ttys has changed, and SIGTERM's which
 * are used to shutdown the system.
 */
multiple()
{
	register struct tab *p;
	register pid;

	sigvec(SIGHUP, &mvec, (struct sigvec *)0);
	for (EVER) {
		pid = wait((int *)0);
		if (pid == -1)
			return;
		for (ALL)
			if (p->pid == pid || p->pid == -1) {
				rmut(p);
				dfork(p);
			}
	}
}

/*
 * Merge current contents of ttys file
 * into in-core table of configured tty lines.
 * Entered as signal handler for SIGHUP.
 */
#define	FOUND	1
#define	CHANGE	2

merge()
{
	register struct tab *p;

	fi = open(ifile, 0);
	if (fi < 0) {
		int f, oerrno = errno;
		extern char *sys_errlist[];

		f = open("/dev/console", O_WRONLY);
		write(f, "init: can't open ", 17);
		write(f, ifile, strlen(ifile));
		write(f, ": ", 2);
		write(f, sys_errlist[oerrno],
			strlen(sys_errlist[oerrno]));
		write(f, "\n", 1);
		close(f);
		if ((f = open("/dev/tty", 2)) >= 0) {
			ioctl(f, TIOCNOTTY, 0);
			close(f);
		}
		return;
	}
	for (ALL)
		p->xflag = 0;
	while (rline()) {
		for (ALL) {
			if (SCMPN(p->line, line.line))
				continue;
			p->xflag |= FOUND;
			if (line.comn != p->comn) {
				p->xflag |= CHANGE;
				p->comn = line.comn;
			}
			goto contin1;
		}
		for (ALL) {
			if (p->line[0] != 0)
				continue;
			SCPYN(p->line, line.line);
			p->xflag |= FOUND|CHANGE;
			p->comn = line.comn;
			goto contin1;
		}
	contin1:
		;
	}
	close(fi);
	for (ALL) {
		if ((p->xflag&FOUND) == 0) {
			term(p);
			p->line[0] = 0;
		}
		if (p->xflag&CHANGE) {
			term(p);
			dfork(p);
		}
	}
}

term(p)
	register struct tab *p;
{

	if (p->pid != 0) {
		rmut(p);
		kill(p->pid, SIGKILL);
	}
	p->pid = 0;
}

rline()
{
	register c, i;

loop:
	c = get();
	if (c < 0)
		return(0);
	if (c == 0)
		goto loop;
	line.flag = c;
	c = get();
	if (c <= 0)
		goto loop;
	line.comn = c;
	SCPYN(line.line, "");
	for (i = 0; i < LINSIZ; i++) {
		c = get();
		if (c <= 0)
			break;
		line.line[i] = c;
	}
	while (c > 0)
		c = get();
	if (line.line[0] == 0)
		goto loop;
	if (line.flag == '0')
		goto loop;
	strcpy(tty, dev);
	strncat(tty, line.line, LINSIZ);
	if (access(tty, 06) < 0)
		goto loop;
	return (1);
}

get()
{
	char b;

	if (read(fi, &b, 1) != 1)
		return (-1);
	if (b == '\n')
		return (0);
	return (b);
}

dfork(p)
	struct tab *p;
{
	register pid;
	time_t t;
	int dowait = 0;
	extern char *sys_errlist[];

	time(&t);
	p->gettycnt++;
	if ((t - p->gettytime) >= 60) {
		p->gettytime = t;
		p->gettycnt = 1;
	} else {
		if (p->gettycnt >= 5) {
			dowait = 1;
			p->gettytime = t;
			p->gettycnt = 1;
		}
	}
	pid = fork();
	if (pid == 0) {
		int oerrno, f;
		extern int errno;

		signal(SIGTERM, SIG_DFL);
		signal(SIGHUP, SIG_IGN);
		strcpy(tty, dev);
		strncat(tty, p->line, LINSIZ);
		if (dowait) {
			f = open("/dev/console", O_WRONLY);
			write(f, "init: ", 6);
			write(f, tty, strlen(tty));
			write(f, ": getty failing, sleeping\n\r", 27);
			close(f);
			sleep(30);
			if ((f = open("/dev/tty", O_RDWR)) >= 0) {
				ioctl(f, TIOCNOTTY, 0);
				close(f);
			}
		}
		chown(tty, 0, 0);
		chmod(tty, 0622);
		if (open(tty, O_RDWR) < 0) {
			int repcnt = 0;
			do {
				oerrno = errno;
				if (repcnt % 10 == 0) {
					f = open("/dev/console", O_WRONLY);
					write(f, "init: ", 6);
					write(f, tty, strlen(tty));
					write(f, ": ", 2);
					write(f, sys_errlist[oerrno],
						strlen(sys_errlist[oerrno]));
					write(f, "\n", 1);
					close(f);
					if ((f = open("/dev/tty", 2)) >= 0) {
						ioctl(f, TIOCNOTTY, 0);
						close(f);
					}
				}
				repcnt++;
				sleep(60);
			} while (open(tty, O_RDWR) < 0);
			exit(0);	/* have wrong control tty, start over */
		}
		vhangup();
		signal(SIGHUP, SIG_DFL);
		(void) open(tty, O_RDWR);
		close(0);
		dup(1);
		dup(0);
		tty[0] = p->comn;
		tty[1] = 0;
		execl(getty, minus, tty, (char *)0);
		exit(0);
	}
	p->pid = pid;
}

/*
 * Remove utmp entry.
 */
rmut(p)
	register struct tab *p;
{
	register f;
	int found = 0;

	f = open(utmp, O_RDWR);
	if (f >= 0) {
		while (read(f, (char *)&wtmp, sizeof(wtmp)) == sizeof(wtmp)) {
			if (SCMPN(wtmp.ut_line, p->line) || wtmp.ut_name[0]==0)
				continue;
			lseek(f, -(long)sizeof(wtmp), 1);
			SCPYN(wtmp.ut_name, "");
			SCPYN(wtmp.ut_host, "");
			time(&wtmp.ut_time);
			write(f, (char *)&wtmp, sizeof(wtmp));
			found++;
		}
		close(f);
	}
	if (found) {
		f = open(wtmpf, O_WRONLY|O_APPEND);
		if (f >= 0) {
			SCPYN(wtmp.ut_line, p->line);
			SCPYN(wtmp.ut_name, "");
			SCPYN(wtmp.ut_host, "");
			time(&wtmp.ut_time);
			write(f, (char *)&wtmp, sizeof(wtmp));
			close(f);
		}
	}
}

reset()
{

	longjmp(sjbuf, 1);
}

jmp_buf	idlebuf;

idlehup()
{

	longjmp(idlebuf, 1);
}

idle()
{
	register struct tab *p;
	register pid;

	signal(SIGHUP, idlehup);
	for (;;) {
		if (setjmp(idlebuf))
			return;
		pid = wait((int *) 0);
		if (pid == -1) {
			sigpause(0);
			continue;
		}
		for (ALL)
			if (p->pid == pid) {
				rmut(p);
				p->pid = -1;
			}
	}
}

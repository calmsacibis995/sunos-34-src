/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char sccsid[] = "@(#)script.c 1.1 86/09/25 SMI"; /* from UCB 5.4 11/13/85 */
#endif not lint

/*
 * script
 */
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sgtty.h>
#include <sys/time.h>
#include <sys/file.h>

char	*getenv();
char	*ctime();
char	*shell;
FILE	*fscript;
int	master;
int	slave;
int	child;
int	subchild;
char	*fname = "typescript";
int	finish();

struct	sgttyb b;
struct	tchars tc;
struct	ltchars lc;
struct	winsize win;
int	lb;
int	l;
char	*line = "/dev/ptyXX";
int	aflg;

main(argc, argv)
	int argc;
	char *argv[];
{

	shell = getenv("SHELL");
	if (shell == 0)
		shell = "/bin/sh";
	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		switch (argv[0][1]) {

		case 'a':
			aflg++;
			break;

		default:
			fprintf(stderr,
			    "usage: script [ -a ] [ typescript ]\n");
			exit(1);
		}
		argc--, argv++;
	}
	if (argc > 0)
		fname = argv[0];
	if ((fscript = fopen(fname, aflg ? "a" : "w")) == NULL) {
		perror(fname);
		fail();
	}
	getmaster();
	printf("Script started, file is %s\n", fname);
	fixtty();

	(void) signal(SIGCHLD, finish);
	child = fork();
	if (child < 0) {
		perror("fork");
		fail();
	}
	if (child == 0) {
		subchild = child = fork();
		if (child < 0) {
			perror("fork");
			fail();
		}
		if (child)
			dooutput();
		else
			doshell();
	}
	doinput();
}

doinput()
{
	char ibuf[BUFSIZ];
	int cc;

	(void) fclose(fscript);
	while ((cc = read(0, ibuf, BUFSIZ)) > 0)
		(void) write(master, ibuf, cc);
	done();
}

#include <sys/wait.h>

finish()
{
	union wait status;
	register int pid;
	register int die = 0;

	while ((pid = wait3(&status, WNOHANG, 0)) > 0)
		if (pid == child)
			die = 1;

	if (die)
		done();
}

dooutput()
{
	time_t tvec;
	char obuf[BUFSIZ];
	int cc;

	(void) close(0);
	tvec = time((time_t *)0);
	fprintf(fscript, "Script started on %s", ctime(&tvec));
	for (;;) {
		cc = read(master, obuf, sizeof (obuf));
		if (cc <= 0)
			break;
		(void) write(1, obuf, cc);
		(void) fwrite(obuf, 1, cc, fscript);
	}
	done();
}

doshell()
{
	int t;

	t = open("/dev/tty", O_RDWR);
	if (t >= 0) {
		(void) ioctl(t, TIOCNOTTY, (char *)0);
		(void) close(t);
	}
	getslave();
	(void) close(master);
	(void) fclose(fscript);
	(void) dup2(slave, 0);
	(void) dup2(slave, 1);
	(void) dup2(slave, 2);
	(void) close(slave);
	execl(shell, "sh", "-i", 0);
	perror(shell);
	fail();
}

fixtty()
{
	struct sgttyb sbuf;

	sbuf = b;
	sbuf.sg_flags |= RAW;
	sbuf.sg_flags &= ~ECHO;
	(void) ioctl(0, TIOCSETP, (char *)&sbuf);
}

fail()
{

	(void) kill(0, SIGTERM);
	done();
}

done()
{
	time_t tvec;

	if (subchild) {
		tvec = time((time_t *)0);
		fprintf(fscript,"\nscript done on %s", ctime(&tvec));
		(void) fclose(fscript);
		(void) close(master);
	} else {
		(void) ioctl(0, TIOCSETP, (char *)&b);
		printf("Script done, file is %s\n", fname);
	}
	exit(0);
}

getmaster()
{
	char *pty, *bank, *cp;
	struct stat stb;

	pty = &line[strlen("/dev/ptyp")];
	for (bank = "pqrs"; *bank; bank++) {
		line[strlen("/dev/pty")] = *bank;
		*pty = '0';
		if (stat(line, &stb) < 0)
			break;
		for (cp = "0123456789abcdef"; *cp; cp++) {
			*pty = *cp;
			master = open(line, O_RDWR);
			if (master >= 0) {
				char *tp = &line[strlen("/dev/")];
				int ok;

				/* verify slave side is usable */
				*tp = 't';
				ok = access(line, R_OK|W_OK) == 0;
				*tp = 'p';
				if (ok) {
				    (void) ioctl(0, TIOCGETP, (char *)&b);
				    (void) ioctl(0, TIOCGETC, (char *)&tc);
				    (void) ioctl(0, TIOCGETD, (char *)&l);
				    (void) ioctl(0, TIOCGLTC, (char *)&lc);
				    (void) ioctl(0, TIOCLGET, (char *)&lb);
				    (void) ioctl(0, TIOCGWINSZ, (char *)&win);
					return;
				}
				(void) close(master);
			}
		}
	}
	fprintf(stderr, "Out of pty's\n");
	fail();
}

getslave()
{

	line[strlen("/dev/")] = 't';
	slave = open(line, O_RDWR);
	if (slave < 0) {
		perror(line);
		fail();
	}
	(void) ioctl(slave, TIOCSETP, (char *)&b);
	(void) ioctl(slave, TIOCSETC, (char *)&tc);
	(void) ioctl(slave, TIOCSLTC, (char *)&lc);
	(void) ioctl(slave, TIOCLSET, (char *)&lb);
	(void) ioctl(slave, TIOCSETD, (char *)&l);
	(void) ioctl(slave, TIOCSWINSZ, (char *)&win);
}
